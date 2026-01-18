#pragma once

#include <cstdint>

#include <ase/types/option.hpp>
#include <ase/types/result.hpp>

namespace ase::types {

/// Invalid entity ID marker (UINT32_MAX). Entity 0 is VALID in EnTT!
constexpr uint32_t InvalidEntityId = UINT32_MAX;

}  // namespace ase::types
