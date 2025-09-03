/*
 * Server Test Shim: UNIX socket control plane to drive mock HAL at runtime
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include "MplaneUplaneConf.h"
#include "mock_hal_control.h"

static const char* kSockPath = "/tmp/haltest.sock";
static int srv_fd = -1;

static void cleanup() {
  if (srv_fd >= 0) close(srv_fd);
  unlink(kSockPath);
}

static void on_sigint(int) { cleanup(); std::exit(0); }

static std::map<std::string, std::string> parse_kv(const std::string& line) {
  std::map<std::string, std::string> kv;
  size_t start = 0;
  while (start < line.size()) {
    size_t end = line.find(' ', start);
    if (end == std::string::npos) end = line.size();
    std::string tok = line.substr(start, end - start);
    size_t eq = tok.find('=');
    if (eq != std::string::npos) {
      kv[tok.substr(0, eq)] = tok.substr(eq + 1);
    } else {
      // positional tokens: use numbered keys
      kv["_pos"] = tok;
    }
    start = end + 1;
  }
  return kv;
}

static int handle_line(int cfd, const std::string& line) {
  if (line.empty()) return 0;
  auto kv = parse_kv(line);
  auto it = kv.find("_pos");
  std::string cmd = (it != kv.end()) ? it->second : "";

  if (cmd == "status") {
    char buf[256];
    if (mock_hal_status(buf, sizeof(buf)) == 0) {
      dprintf(cfd, "%s\n", buf);
    } else {
      dprintf(cfd, "ERR:status\n");
    }
    return 0;
  }

  if (cmd == "sync") {
    std::string state = kv["state"];
    bool locked = (state == "LOCKED") || (state == "locked") || (state == "1");
    mock_hal_set_sync_locked(locked);
    dprintf(cfd, "OK\n");
    return 0;
  }

  if (cmd == "alarm") {
    uint16_t id = (uint16_t)std::strtoul(kv["id"].c_str(), nullptr, 10);
    std::string source = kv["source"]; if (source.empty()) source = "SIM";
    std::string sev = kv["severity"]; int sev_i = 1;
    if (sev == "Critical" || sev == "CRITICAL") sev_i = 0;
    else if (sev == "Major" || sev == "MAJOR") sev_i = 1;
    else if (sev == "Minor" || sev == "MINOR") sev_i = 2;
    else if (sev == "Warning" || sev == "WARNING") sev_i = 3;
    bool cleared = (kv["clear"] == "true" || kv["clear"] == "1");
    std::string text = kv["text"]; if (text.empty()) text = "sim alarm";
    int rc = mock_hal_inject_alarm(id, source.c_str(), sev_i, cleared, text.c_str());
    dprintf(cfd, rc == 0 ? "OK\n" : "ERR:alarm\n");
    return 0;
  }

  if (cmd == "sw") {
    const char* phase = kv["phase"].c_str();
    const char* result = kv["result"].c_str();
    const char* slot = kv.count("slot") ? kv["slot"].c_str() : nullptr;
    const char* file = kv.count("file") ? kv["file"].c_str() : nullptr;
    mock_hal_set_sw_result_ext(phase, result, slot, file);
    dprintf(cfd, "OK\n");
    return 0;
  }

  if (cmd == "pm") {
    int rc = mock_hal_trigger_pm(kv["object"].c_str(), kv["out"].c_str());
    dprintf(cfd, rc == 0 ? "OK\n" : "ERR:pm\n");
    return 0;
  }

  if (cmd == "uplane") {
    std::string dir = kv["dir"]; // "tx" or "rx"
    std::string name = kv["name"]; std::string st = kv["state"]; if (name.empty() || st.empty()) {
      dprintf(cfd, "ERR:uplane args\n"); return 0;
    }
    if (dir == "tx") {
      int rc = halmplane_tx_carrier_state_change(name.c_str(), 0, 0, 0.0, st.c_str(), 1);
      dprintf(cfd, rc == 0 ? "OK\n" : "ERR:tx\n");
    } else {
      int rc = halmplane_rx_carrier_state_change(name.c_str(), 0, 0, 0.0, st.c_str(), 1);
      dprintf(cfd, rc == 0 ? "OK\n" : "ERR:rx\n");
    }
    return 0;
  }

  dprintf(cfd, "ERR:unknown\n");
  return 0;
}

extern "C" int halmplane_init(void*);
extern "C" int halmplane_exit();

int main() {
  std::signal(SIGINT, on_sigint);
  std::signal(SIGTERM, on_sigint);

  halmplane_init(nullptr);

  srv_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (srv_fd < 0) {
    std::perror("socket");
    return 1;
  }
  unlink(kSockPath);

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", kSockPath);
  if (bind(srv_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    std::perror("bind");
    cleanup();
    return 2;
  }
  if (listen(srv_fd, 4) < 0) {
    std::perror("listen");
    cleanup();
    return 3;
  }

  std::fprintf(stderr, "server-test-shim listening on %s\n", kSockPath);

  while (true) {
    int cfd = accept(srv_fd, nullptr, nullptr);
    if (cfd < 0) {
      if (errno == EINTR) continue;
      std::perror("accept");
      break;
    }
    char line[512];
    ssize_t n;
    size_t pos = 0;
    while ((n = read(cfd, line + pos, sizeof(line) - pos - 1)) > 0) {
      pos += (size_t)n;
      line[pos] = '\0';
      char* start = line;
      char* nl;
      while ((nl = strchr(start, '\n')) != nullptr) {
        *nl = '\0';
        handle_line(cfd, std::string(start));
        start = nl + 1;
      }
      // move remaining partial line to start
      size_t rem = (line + pos) - start;
      memmove(line, start, rem);
      pos = rem;
    }
    close(cfd);
  }

  cleanup();
  halmplane_exit();
  return 0;
}
