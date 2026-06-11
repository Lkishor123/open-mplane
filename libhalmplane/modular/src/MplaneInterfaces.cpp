/*
 * Copyright (c) Linux Foundation and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#include "MplaneInterfaces.h"
#include "ModuleLoader.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace {
const char* mockRuLogPath()
{
  const char* path = std::getenv("MPLANE_MOCK_RU_LOG");
  return path && path[0] != '\0' ? path : "/tmp/open-mplane-mock-ru.log";
}

void logMockRuRequest(const char* action, const std::string& detail)
{
  std::ofstream log(mockRuLogPath(), std::ios::app);
  if (log) {
    log << "mock-ru halmplane " << action << " " << detail << '\n';
  }
}
}

halmplane_error_t halmplane_interface_update(interface_t* interface)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update",
      std::string("name=") + (interface && interface->name ? interface->name : "<null>"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update(interface_t*)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(interface_t*)) fptr)(interface);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_description(const char* name, const char* description)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_description",
      std::string("name=") + (name ? name : "<null>") +
          " description=" + (description ? description : "<null>"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_description(const char*, const char*)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, const char*)) fptr)(name, description);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_type(const char* name, const char* type)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_type",
      std::string("name=") + (name ? name : "<null>") +
          " type=" + (type ? type : "<null>"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_type(const char*, const char*)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, const char*)) fptr)(name, type);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_enabled(const char* name, bool enabled)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_enabled",
      std::string("name=") + (name ? name : "<null>") +
          " enabled=" + (enabled ? "true" : "false"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_enabled(const char*, bool)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, bool)) fptr)(name, enabled);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_l2_mtu(const char* name, int l2Mtu)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_l2_mtu",
      std::string("name=") + (name ? name : "<null>") +
          " l2Mtu=" + std::to_string(l2Mtu));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_l2_mtu(const char*, int)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, int)) fptr)(name, l2Mtu);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_vlan_tagging(const char* name, bool vlanTagging)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_vlan_tagging",
      std::string("name=") + (name ? name : "<null>") +
          " vlanTagging=" + (vlanTagging ? "true" : "false"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_vlan_tagging(const char*, bool)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, bool)) fptr)(name, vlanTagging);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_base_interface(const char* name, const char* baseInterface)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_base_interface",
      std::string("name=") + (name ? name : "<null>") +
          " baseInterface=" + (baseInterface ? baseInterface : "<null>"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_base_interface(const char*, const char*)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, const char*)) fptr)(name, baseInterface);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_vlan_id(const char* name, int vlanId)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_vlan_id",
      std::string("name=") + (name ? name : "<null>") +
          " vlanId=" + std::to_string(vlanId));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_vlan_id(const char*, int)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, int)) fptr)(name, vlanId);
    }
  return status;
}

halmplane_error_t halmplane_interface_update_mac_address(const char* name, const char* macAddress)
{
  halmplane_error_t status = UNAVAILABLE;
  void* fptr;

  logMockRuRequest(
      "interface_update_mac_address",
      std::string("name=") + (name ? name : "<null>") +
          " macAddress=" + (macAddress ? macAddress : "<null>"));

  fptr = _loader()->get("halmplane_error_t halmplane_interface_update_mac_address(const char*, const char*)");
  if(fptr == NULL)
    {
      status = UNIMPLEMENTED;
    }
  else
    {
      status = ((halmplane_error_t (*)(const char*, const char*)) fptr)(name, macAddress);
    }  
  return status;
}
