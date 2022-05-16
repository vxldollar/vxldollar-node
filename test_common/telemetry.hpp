#pragma once

#include <cstdint>

namespace vxldollar
{
class keypair;
class network_params;
class telemetry_data;

void compare_default_telemetry_response_data_excluding_signature (vxldollar::telemetry_data const & telemetry_data_a, vxldollar::network_params const & network_params_a, uint64_t bandwidth_limit_a, uint64_t active_difficulty_a);
void compare_default_telemetry_response_data (vxldollar::telemetry_data const & telemetry_data_a, vxldollar::network_params const & network_params_a, uint64_t bandwidth_limit_a, uint64_t active_difficulty_a, vxldollar::keypair const & node_id_a);
}