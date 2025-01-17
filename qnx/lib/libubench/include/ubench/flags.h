#ifndef UBENCH_ENUM_FLAGS_H
#define UBENCH_ENUM_FLAGS_H

#include <bitset>
#include <type_traits>

namespace ubench {

/// @brief generic class that can manipulate raw bits, often provided by the
/// Operating System.
///
/// @tparam BitType the class enum, or enum that defines the bits. The values of
/// the enums define the flag value, and not the bit position.
template <typename bit_type,
    std::enable_if_t<std::is_enum_v<bit_type>, bool> = true>
class flags {
 public:
  /// @brief The underlying type for the flags.
  using mask_type = std::underlying_type_t<bit_type>;

  /// @brief default constructor.
  constexpr flags() noexcept : mask_(0) {}

  /// @brief construct from an enum value.
  ///
  /// @param bit the enum value to set.
  constexpr flags(bit_type bit) noexcept : mask_{static_cast<mask_type>(bit)} {}

  /// @brief copy constructur.
  ///
  /// @param rhs the flags to copy.
  constexpr flags(const flags& rhs) noexcept = default;

  /// @brief construct from the underlying type of the enum (raw).
  ///
  /// @param bits the underlying (integer) value.
  constexpr explicit flags(mask_type bits) noexcept : mask_(bits) {}

  /// @brief Copy assignment operator.
  ///
  /// @param rhs the flags to assign to this object.
  ///
  /// @return the flags that are a copy of rhs.
  constexpr auto operator=(const flags& rhs) noexcept -> flags& = default;

  /// @brief Check if no bits are set.
  ///
  /// @return true if no bits are set, else false.
  [[nodiscard]] constexpr auto operator!() const noexcept -> bool {
    return !mask_;
  }

  /// @brief Clears out bits that are not set by rhs (bit masking).
  ///
  /// @param rhs The bits that should mask the the current flag.
  ///
  /// @return the sets of bits in the current flags that are set where also the
  /// bits in rhs are set.
  [[nodiscard]] constexpr auto operator&(const flags& rhs) const noexcept
      -> flags {
    return flags(mask_ & rhs.mask_);
  }

  /// @brief Sets bits to the flags that are set by the rhs.
  ///
  /// @param rhs the bits to set.
  ///
  /// @return the new set of bits of the current mask with bits of rhs also set.
  [[nodiscard]] constexpr auto operator|(const flags& rhs) const noexcept
      -> flags {
    return flags(mask_ | rhs.mask_);
  }

  /// @brief Invert all bits in the current flags with rhs.
  ///
  /// @param rhs the sets of bits that define which should be inverted.
  ///
  /// @return the new set of bits in the current mask that are inverted defined
  /// by rhs.
  [[nodiscard]] constexpr auto operator^(const flags& rhs) const noexcept
      -> flags {
    return flags(mask_ ^ rhs.mask_);
  }

  /// @brief Invert all bits in the current flags up to the current mask.
  ///
  /// @return the set of all bits that have been inverted.
  [[nodiscard]] constexpr auto operator~() const noexcept -> flags {
    return flags(mask_ ^ static_cast<mask_type>(-1));
  }

  /// @brief Test equality for the flags.
  ///
  /// @param rhs the flags to compare against.
  ///
  /// @return true if equal, false otherwise. All flags are tested, not just the
  /// ones defined in the bit_type.
  [[nodiscard]] constexpr auto operator==(const flags& rhs) const noexcept
      -> bool {
    return mask_ == rhs.mask_;
  }

  /// @brief Test inequality for the flags.
  ///
  /// @param rhs the flags to compare against.
  ///
  /// @return true if unequal, false otherwise. All flags are tested, not just
  /// the ones defined in the bit_type.
  [[nodiscard]] constexpr auto operator!=(const flags& rhs) const noexcept
      -> bool {
    return !operator==(rhs);
  }

  /// @brief Assignment and bit set operator.
  ///
  /// @param rhs the bits to set.
  //
  /// @return the object with the bits set.
  constexpr auto operator|=(const flags& rhs) noexcept -> flags& {
    mask_ |= rhs.mask_;
    return *this;
  }

  /// @brief Assignment and bit mask operator.
  ///
  /// @param rhs the sets of flags that should be set.
  ///
  /// @return the object with the bits masked.
  constexpr auto operator&=(const flags& rhs) noexcept -> flags& {
    mask_ &= rhs.mask_;
    return *this;
  }

  /// @brief Assignment and bit invert operator.
  ///
  /// @param rhs the sets of flags that should invert the current flags.
  ///
  /// @return the new flags with the required bits inverted.
  constexpr auto operator^=(const flags& rhs) noexcept -> flags& {
    mask_ ^= rhs.mask_;
    return *this;
  }

  /// @brief Assignment and bit set operator.
  ///
  /// @param rhs the bits to set.
  //
  /// @return the object with the bits set.
  constexpr auto operator|=(const bit_type& rhs) noexcept -> flags& {
    mask_ |= static_cast<mask_type>(rhs);
    return *this;
  }

  /// @brief Assignment and bit mask operator.
  ///
  /// @param rhs the sets of flags that should be set.
  ///
  /// @return the object with the bits masked.
  constexpr auto operator&=(const bit_type& rhs) noexcept -> flags& {
    mask_ &= static_cast<mask_type>(rhs);
    return *this;
  }

  /// @brief Assignment and bit invert operator.
  ///
  /// @param rhs the sets of flags that should invert the current flags.
  ///
  /// @return the new flags with the required bits inverted.
  constexpr auto operator^=(const bit_type& rhs) noexcept -> flags& {
    mask_ ^= static_cast<mask_type>(rhs);
    return *this;
  }

  /// @brief Bool cast operator
  ///
  /// @return true if at least one bit is set (even if not defined in the enum).
  [[nodiscard]] explicit constexpr operator bool() const noexcept {
    return !!mask_;
  }

  /// @brief cast operator to the underlying type
  ///
  /// @return the value of the flags (all) from the underlying type.
  [[nodiscard]] explicit constexpr operator mask_type() const noexcept {
    return mask_;
  }

  /// @brief Return the underlying data in the underlying data type.
  ///
  /// @return the raw flags.
  [[nodiscard]] constexpr auto data() const noexcept -> mask_type {
    return mask_;
  }

 private:
  mask_type mask_;
};

}  // namespace ubench

/// @brief Mask two flags together.
///
/// @tparam bit_type the enum type of the flags.
///
/// @param bit the enum flag to mask.
///
/// @param flags the flags to mask with.
///
/// @return the result of the mask.
template <typename bit_type,
    std::enable_if_t<std::is_enum_v<bit_type>, bool> = true>
[[nodiscard]] inline constexpr auto operator&(bit_type bit,
    const ubench::flags<bit_type>& flags) noexcept -> ubench::flags<bit_type> {
  return flags.operator&(bit);
}

/// @brief Combine two flags together.
///
/// @tparam bit_type the enum type of the flags.
///
/// @param bit the enum flag to combine.
///
/// @param flags the flags to combine with.
///
/// @return the result of the combine.
template <typename bit_type,
    std::enable_if_t<std::is_enum_v<bit_type>, bool> = true>
[[nodiscard]] inline constexpr auto operator|(bit_type bit,
    const ubench::flags<bit_type>& flags) noexcept -> ubench::flags<bit_type> {
  return flags.operator|(bit);
}

/// @brief Exclusive-or two flags together.
///
/// @tparam bit_type the enum type of the flags.
///
/// @param bit the enum flag to test and invert.
///
/// @param flags the flags to invert with.
///
/// @return the result of the exclusive-or.
template <typename bit_type,
    std::enable_if_t<std::is_enum_v<bit_type>, bool> = true>
[[nodiscard]] inline constexpr auto operator^(bit_type bit,
    const ubench::flags<bit_type>& flags) noexcept -> ubench::flags<bit_type> {
  return flags.operator^(bit);
}

#define DEFINE_GLOBAL_FLAG_OPERATORS(E)                               \
  template <typename bit_type,                                        \
      std::enable_if_t<std::is_same_v<bit_type, E>, bool> = true>     \
  [[nodiscard]] inline constexpr auto operator&(                      \
      bit_type lhs, bit_type rhs) noexcept->ubench::flags<bit_type> { \
    return ubench::flags<bit_type>(lhs) & rhs;                        \
  }                                                                   \
                                                                      \
  template <typename bit_type,                                        \
      std::enable_if_t<std::is_same_v<bit_type, E>, bool> = true>     \
  [[nodiscard]] inline constexpr auto operator|(                      \
      bit_type lhs, bit_type rhs) noexcept->ubench::flags<bit_type> { \
    return ubench::flags<bit_type>(lhs) | rhs;                        \
  }                                                                   \
                                                                      \
  template <typename bit_type,                                        \
      std::enable_if_t<std::is_same_v<bit_type, E>, bool> = true>     \
  [[nodiscard]] inline constexpr auto operator^(                      \
      bit_type lhs, bit_type rhs) noexcept->ubench::flags<bit_type> { \
    return ubench::flags<bit_type>(lhs) ^ rhs;                        \
  }                                                                   \
                                                                      \
  template <typename bit_type,                                        \
      std::enable_if_t<std::is_same_v<bit_type, E>, bool> = true>     \
  [[nodiscard]] inline constexpr auto operator~(                      \
      bit_type bit) noexcept->ubench::flags<bit_type> {               \
    return ~(ubench::flags<bit_type>(bit));                           \
  }

#endif
