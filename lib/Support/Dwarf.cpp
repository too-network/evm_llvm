//===-- llvm/Support/Dwarf.cpp - Dwarf Framework ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for generic dwarf information.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Dwarf.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;
using namespace dwarf;

StringRef llvm::dwarf::TagString(unsigned Tag) {
  switch (Tag) {
  default:
    return StringRef();
#define HANDLE_DW_TAG(ID, NAME)                                                \
  case DW_TAG_##NAME:                                                          \
    return "DW_TAG_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getTag(StringRef TagString) {
  return StringSwitch<unsigned>(TagString)
#define HANDLE_DW_TAG(ID, NAME) .Case("DW_TAG_" #NAME, DW_TAG_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(DW_TAG_invalid);
}

StringRef llvm::dwarf::ChildrenString(unsigned Children) {
  switch (Children) {
  case DW_CHILDREN_no:                   return "DW_CHILDREN_no";
  case DW_CHILDREN_yes:                  return "DW_CHILDREN_yes";
  }
  return StringRef();
}

StringRef llvm::dwarf::AttributeString(unsigned Attribute) {
  switch (Attribute) {
  default:
    return StringRef();
#define HANDLE_DW_AT(ID, NAME)                                                \
  case DW_AT_##NAME:                                                          \
    return "DW_AT_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

StringRef llvm::dwarf::FormEncodingString(unsigned Encoding) {
  switch (Encoding) {
  case DW_FORM_addr:                     return "DW_FORM_addr";
  case DW_FORM_block2:                   return "DW_FORM_block2";
  case DW_FORM_block4:                   return "DW_FORM_block4";
  case DW_FORM_data2:                    return "DW_FORM_data2";
  case DW_FORM_data4:                    return "DW_FORM_data4";
  case DW_FORM_data8:                    return "DW_FORM_data8";
  case DW_FORM_string:                   return "DW_FORM_string";
  case DW_FORM_block:                    return "DW_FORM_block";
  case DW_FORM_block1:                   return "DW_FORM_block1";
  case DW_FORM_data1:                    return "DW_FORM_data1";
  case DW_FORM_flag:                     return "DW_FORM_flag";
  case DW_FORM_sdata:                    return "DW_FORM_sdata";
  case DW_FORM_strp:                     return "DW_FORM_strp";
  case DW_FORM_udata:                    return "DW_FORM_udata";
  case DW_FORM_ref_addr:                 return "DW_FORM_ref_addr";
  case DW_FORM_ref1:                     return "DW_FORM_ref1";
  case DW_FORM_ref2:                     return "DW_FORM_ref2";
  case DW_FORM_ref4:                     return "DW_FORM_ref4";
  case DW_FORM_ref8:                     return "DW_FORM_ref8";
  case DW_FORM_ref_udata:                return "DW_FORM_ref_udata";
  case DW_FORM_indirect:                 return "DW_FORM_indirect";
  case DW_FORM_sec_offset:               return "DW_FORM_sec_offset";
  case DW_FORM_exprloc:                  return "DW_FORM_exprloc";
  case DW_FORM_flag_present:             return "DW_FORM_flag_present";
  case DW_FORM_ref_sig8:                 return "DW_FORM_ref_sig8";

    // DWARF5 Fission Extension Forms
  case DW_FORM_GNU_addr_index:           return "DW_FORM_GNU_addr_index";
  case DW_FORM_GNU_str_index:            return "DW_FORM_GNU_str_index";

  // Alternate debug sections proposal (output of "dwz" tool).
  case DW_FORM_GNU_ref_alt:              return "DW_FORM_GNU_ref_alt";
  case DW_FORM_GNU_strp_alt:             return "DW_FORM_GNU_strp_alt";
  }
  return StringRef();
}

StringRef llvm::dwarf::OperationEncodingString(unsigned Encoding) {
  switch (Encoding) {
  default:
    return StringRef();
#define HANDLE_DW_OP(ID, NAME)                                                 \
  case DW_OP_##NAME:                                                           \
    return "DW_OP_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getOperationEncoding(StringRef OperationEncodingString) {
  return StringSwitch<unsigned>(OperationEncodingString)
#define HANDLE_DW_OP(ID, NAME) .Case("DW_OP_" #NAME, DW_OP_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(0);
}

StringRef llvm::dwarf::AttributeEncodingString(unsigned Encoding) {
  switch (Encoding) {
  default:
    return StringRef();
#define HANDLE_DW_ATE(ID, NAME)                                                \
  case DW_ATE_##NAME:                                                          \
    return "DW_ATE_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getAttributeEncoding(StringRef EncodingString) {
  return StringSwitch<unsigned>(EncodingString)
#define HANDLE_DW_ATE(ID, NAME) .Case("DW_ATE_" #NAME, DW_ATE_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(0);
}

StringRef llvm::dwarf::DecimalSignString(unsigned Sign) {
  switch (Sign) {
  case DW_DS_unsigned:                   return "DW_DS_unsigned";
  case DW_DS_leading_overpunch:          return "DW_DS_leading_overpunch";
  case DW_DS_trailing_overpunch:         return "DW_DS_trailing_overpunch";
  case DW_DS_leading_separate:           return "DW_DS_leading_separate";
  case DW_DS_trailing_separate:          return "DW_DS_trailing_separate";
  }
  return StringRef();
}

StringRef llvm::dwarf::EndianityString(unsigned Endian) {
  switch (Endian) {
  case DW_END_default:                   return "DW_END_default";
  case DW_END_big:                       return "DW_END_big";
  case DW_END_little:                    return "DW_END_little";
  case DW_END_lo_user:                   return "DW_END_lo_user";
  case DW_END_hi_user:                   return "DW_END_hi_user";
  }
  return StringRef();
}

StringRef llvm::dwarf::AccessibilityString(unsigned Access) {
  switch (Access) {
  // Accessibility codes
  case DW_ACCESS_public:                 return "DW_ACCESS_public";
  case DW_ACCESS_protected:              return "DW_ACCESS_protected";
  case DW_ACCESS_private:                return "DW_ACCESS_private";
  }
  return StringRef();
}

StringRef llvm::dwarf::VisibilityString(unsigned Visibility) {
  switch (Visibility) {
  case DW_VIS_local:                     return "DW_VIS_local";
  case DW_VIS_exported:                  return "DW_VIS_exported";
  case DW_VIS_qualified:                 return "DW_VIS_qualified";
  }
  return StringRef();
}

StringRef llvm::dwarf::VirtualityString(unsigned Virtuality) {
  switch (Virtuality) {
  default:
    return StringRef();
#define HANDLE_DW_VIRTUALITY(ID, NAME)                                         \
  case DW_VIRTUALITY_##NAME:                                                   \
    return "DW_VIRTUALITY_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getVirtuality(StringRef VirtualityString) {
  return StringSwitch<unsigned>(VirtualityString)
#define HANDLE_DW_VIRTUALITY(ID, NAME)                                         \
  .Case("DW_VIRTUALITY_" #NAME, DW_VIRTUALITY_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(DW_VIRTUALITY_invalid);
}

StringRef llvm::dwarf::LanguageString(unsigned Language) {
  switch (Language) {
  default:
    return StringRef();
#define HANDLE_DW_LANG(ID, NAME)                                               \
  case DW_LANG_##NAME:                                                         \
    return "DW_LANG_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getLanguage(StringRef LanguageString) {
  return StringSwitch<unsigned>(LanguageString)
#define HANDLE_DW_LANG(ID, NAME) .Case("DW_LANG_" #NAME, DW_LANG_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(0);
}

StringRef llvm::dwarf::CaseString(unsigned Case) {
  switch (Case) {
  case DW_ID_case_sensitive:             return "DW_ID_case_sensitive";
  case DW_ID_up_case:                    return "DW_ID_up_case";
  case DW_ID_down_case:                  return "DW_ID_down_case";
  case DW_ID_case_insensitive:           return "DW_ID_case_insensitive";
  }
  return StringRef();
}

StringRef llvm::dwarf::ConventionString(unsigned CC) {
  switch (CC) {
  default:
    return StringRef();
#define HANDLE_DW_CC(ID, NAME)                                               \
  case DW_CC_##NAME:                                                         \
    return "DW_CC_" #NAME;
#include "llvm/Support/Dwarf.def"
  }
}

unsigned llvm::dwarf::getCallingConvention(StringRef CCString) {
  return StringSwitch<unsigned>(CCString)
#define HANDLE_DW_CC(ID, NAME) .Case("DW_CC_" #NAME, DW_CC_##NAME)
#include "llvm/Support/Dwarf.def"
      .Default(0);
}

StringRef llvm::dwarf::InlineCodeString(unsigned Code) {
  switch (Code) {
  case DW_INL_not_inlined:               return "DW_INL_not_inlined";
  case DW_INL_inlined:                   return "DW_INL_inlined";
  case DW_INL_declared_not_inlined:      return "DW_INL_declared_not_inlined";
  case DW_INL_declared_inlined:          return "DW_INL_declared_inlined";
  }
  return StringRef();
}

StringRef llvm::dwarf::ArrayOrderString(unsigned Order) {
  switch (Order) {
  case DW_ORD_row_major:                 return "DW_ORD_row_major";
  case DW_ORD_col_major:                 return "DW_ORD_col_major";
  }
  return StringRef();
}

StringRef llvm::dwarf::DiscriminantString(unsigned Discriminant) {
  switch (Discriminant) {
  case DW_DSC_label:                     return "DW_DSC_label";
  case DW_DSC_range:                     return "DW_DSC_range";
  }
  return StringRef();
}

StringRef llvm::dwarf::LNStandardString(unsigned Standard) {
  switch (Standard) {
  case DW_LNS_copy:                      return "DW_LNS_copy";
  case DW_LNS_advance_pc:                return "DW_LNS_advance_pc";
  case DW_LNS_advance_line:              return "DW_LNS_advance_line";
  case DW_LNS_set_file:                  return "DW_LNS_set_file";
  case DW_LNS_set_column:                return "DW_LNS_set_column";
  case DW_LNS_negate_stmt:               return "DW_LNS_negate_stmt";
  case DW_LNS_set_basic_block:           return "DW_LNS_set_basic_block";
  case DW_LNS_const_add_pc:              return "DW_LNS_const_add_pc";
  case DW_LNS_fixed_advance_pc:          return "DW_LNS_fixed_advance_pc";
  case DW_LNS_set_prologue_end:          return "DW_LNS_set_prologue_end";
  case DW_LNS_set_epilogue_begin:        return "DW_LNS_set_epilogue_begin";
  case DW_LNS_set_isa:                   return "DW_LNS_set_isa";
  }
  return StringRef();
}

StringRef llvm::dwarf::LNExtendedString(unsigned Encoding) {
  switch (Encoding) {
  // Line Number Extended Opcode Encodings
  case DW_LNE_end_sequence:              return "DW_LNE_end_sequence";
  case DW_LNE_set_address:               return "DW_LNE_set_address";
  case DW_LNE_define_file:               return "DW_LNE_define_file";
  case DW_LNE_set_discriminator:         return "DW_LNE_set_discriminator";
  case DW_LNE_lo_user:                   return "DW_LNE_lo_user";
  case DW_LNE_hi_user:                   return "DW_LNE_hi_user";
  }
  return StringRef();
}

StringRef llvm::dwarf::MacinfoString(unsigned Encoding) {
  switch (Encoding) {
  // Macinfo Type Encodings
  case DW_MACINFO_define:                return "DW_MACINFO_define";
  case DW_MACINFO_undef:                 return "DW_MACINFO_undef";
  case DW_MACINFO_start_file:            return "DW_MACINFO_start_file";
  case DW_MACINFO_end_file:              return "DW_MACINFO_end_file";
  case DW_MACINFO_vendor_ext:            return "DW_MACINFO_vendor_ext";
  case DW_MACINFO_invalid:               return "DW_MACINFO_invalid";
  }
  return StringRef();
}

unsigned llvm::dwarf::getMacinfo(StringRef MacinfoString) {
  return StringSwitch<unsigned>(MacinfoString)
      .Case("DW_MACINFO_define", DW_MACINFO_define)
      .Case("DW_MACINFO_undef", DW_MACINFO_undef)
      .Case("DW_MACINFO_start_file", DW_MACINFO_start_file)
      .Case("DW_MACINFO_end_file", DW_MACINFO_end_file)
      .Case("DW_MACINFO_vendor_ext", DW_MACINFO_vendor_ext)
      .Default(DW_MACINFO_invalid);
}

StringRef llvm::dwarf::CallFrameString(unsigned Encoding) {
  switch (Encoding) {
  case DW_CFA_nop:                       return "DW_CFA_nop";
  case DW_CFA_advance_loc:               return "DW_CFA_advance_loc";
  case DW_CFA_offset:                    return "DW_CFA_offset";
  case DW_CFA_restore:                   return "DW_CFA_restore";
  case DW_CFA_set_loc:                   return "DW_CFA_set_loc";
  case DW_CFA_advance_loc1:              return "DW_CFA_advance_loc1";
  case DW_CFA_advance_loc2:              return "DW_CFA_advance_loc2";
  case DW_CFA_advance_loc4:              return "DW_CFA_advance_loc4";
  case DW_CFA_offset_extended:           return "DW_CFA_offset_extended";
  case DW_CFA_restore_extended:          return "DW_CFA_restore_extended";
  case DW_CFA_undefined:                 return "DW_CFA_undefined";
  case DW_CFA_same_value:                return "DW_CFA_same_value";
  case DW_CFA_register:                  return "DW_CFA_register";
  case DW_CFA_remember_state:            return "DW_CFA_remember_state";
  case DW_CFA_restore_state:             return "DW_CFA_restore_state";
  case DW_CFA_def_cfa:                   return "DW_CFA_def_cfa";
  case DW_CFA_def_cfa_register:          return "DW_CFA_def_cfa_register";
  case DW_CFA_def_cfa_offset:            return "DW_CFA_def_cfa_offset";
  case DW_CFA_def_cfa_expression:        return "DW_CFA_def_cfa_expression";
  case DW_CFA_expression:                return "DW_CFA_expression";
  case DW_CFA_offset_extended_sf:        return "DW_CFA_offset_extended_sf";
  case DW_CFA_def_cfa_sf:                return "DW_CFA_def_cfa_sf";
  case DW_CFA_def_cfa_offset_sf:         return "DW_CFA_def_cfa_offset_sf";
  case DW_CFA_val_offset:                return "DW_CFA_val_offset";
  case DW_CFA_val_offset_sf:             return "DW_CFA_val_offset_sf";
  case DW_CFA_val_expression:            return "DW_CFA_val_expression";
  case DW_CFA_MIPS_advance_loc8:         return "DW_CFA_MIPS_advance_loc8";
  case DW_CFA_GNU_window_save:           return "DW_CFA_GNU_window_save";
  case DW_CFA_GNU_args_size:             return "DW_CFA_GNU_args_size";
  case DW_CFA_lo_user:                   return "DW_CFA_lo_user";
  case DW_CFA_hi_user:                   return "DW_CFA_hi_user";
  }
  return StringRef();
}

StringRef llvm::dwarf::ApplePropertyString(unsigned Prop) {
  switch (Prop) {
  case DW_APPLE_PROPERTY_readonly:
    return "DW_APPLE_PROPERTY_readonly";
  case DW_APPLE_PROPERTY_getter:
    return "DW_APPLE_PROPERTY_getter";
  case DW_APPLE_PROPERTY_assign:
    return "DW_APPLE_PROPERTY_assign";
  case DW_APPLE_PROPERTY_readwrite:
    return "DW_APPLE_PROPERTY_readwrite";
  case DW_APPLE_PROPERTY_retain:
    return "DW_APPLE_PROPERTY_retain";
  case DW_APPLE_PROPERTY_copy:
    return "DW_APPLE_PROPERTY_copy";
  case DW_APPLE_PROPERTY_nonatomic:
    return "DW_APPLE_PROPERTY_nonatomic";
  case DW_APPLE_PROPERTY_setter:
    return "DW_APPLE_PROPERTY_setter";
  case DW_APPLE_PROPERTY_atomic:
    return "DW_APPLE_PROPERTY_atomic";
  case DW_APPLE_PROPERTY_weak:
    return "DW_APPLE_PROPERTY_weak";
  case DW_APPLE_PROPERTY_strong:
    return "DW_APPLE_PROPERTY_strong";
  case DW_APPLE_PROPERTY_unsafe_unretained:
    return "DW_APPLE_PROPERTY_unsafe_unretained";
  case DW_APPLE_PROPERTY_nullability:
    return "DW_APPLE_PROPERTY_nullability";
  case DW_APPLE_PROPERTY_null_resettable:
    return "DW_APPLE_PROPERTY_null_resettable";
  case DW_APPLE_PROPERTY_class:
    return "DW_APPLE_PROPERTY_class";
  }
  return StringRef();
}

StringRef llvm::dwarf::AtomTypeString(unsigned AT) {
  switch (AT) {
  case dwarf::DW_ATOM_null:
    return "DW_ATOM_null";
  case dwarf::DW_ATOM_die_offset:
    return "DW_ATOM_die_offset";
  case DW_ATOM_cu_offset:
    return "DW_ATOM_cu_offset";
  case DW_ATOM_die_tag:
    return "DW_ATOM_die_tag";
  case DW_ATOM_type_flags:
    return "DW_ATOM_type_flags";
  }
  return StringRef();
}

StringRef llvm::dwarf::GDBIndexEntryKindString(GDBIndexEntryKind Kind) {
  switch (Kind) {
  case GIEK_NONE:
    return "NONE";
  case GIEK_TYPE:
    return "TYPE";
  case GIEK_VARIABLE:
    return "VARIABLE";
  case GIEK_FUNCTION:
    return "FUNCTION";
  case GIEK_OTHER:
    return "OTHER";
  case GIEK_UNUSED5:
    return "UNUSED5";
  case GIEK_UNUSED6:
    return "UNUSED6";
  case GIEK_UNUSED7:
    return "UNUSED7";
  }
  llvm_unreachable("Unknown GDBIndexEntryKind value");
}

StringRef
llvm::dwarf::GDBIndexEntryLinkageString(GDBIndexEntryLinkage Linkage) {
  switch (Linkage) {
  case GIEL_EXTERNAL:
    return "EXTERNAL";
  case GIEL_STATIC:
    return "STATIC";
  }
  llvm_unreachable("Unknown GDBIndexEntryLinkage value");
}

StringRef llvm::dwarf::AttributeValueString(uint16_t Attr, unsigned Val) {
  switch (Attr) {
  case DW_AT_accessibility:
    return AccessibilityString(Val);
  case DW_AT_virtuality:
    return VirtualityString(Val);
  case DW_AT_language:
    return LanguageString(Val);
  case DW_AT_encoding:
    return AttributeEncodingString(Val);
  case DW_AT_decimal_sign:
    return DecimalSignString(Val);
  case DW_AT_endianity:
    return EndianityString(Val);
  case DW_AT_visibility:
    return VisibilityString(Val);
  case DW_AT_identifier_case:
    return CaseString(Val);
  case DW_AT_calling_convention:
    return ConventionString(Val);
  case DW_AT_inline:
    return InlineCodeString(Val);
  case DW_AT_ordering:
    return ArrayOrderString(Val);
  case DW_AT_discr_value:
    return DiscriminantString(Val);
  }

  return StringRef();
}
