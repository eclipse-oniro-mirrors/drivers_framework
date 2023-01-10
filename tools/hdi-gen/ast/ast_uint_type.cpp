/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_uint_type.h"

namespace OHOS {
namespace HDI {
bool ASTUintType::IsUintType()
{
    return true;
}

String ASTUintType::ToString()
{
    return "unsigned int";
}

TypeKind ASTUintType::GetTypeKind()
{
    return TypeKind::TYPE_UINT;
}

String ASTUintType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "uint32_t";
        case TypeMode::PARAM_IN:
            return "uint32_t";
        case TypeMode::PARAM_OUT:
            return "uint32_t*";
        case TypeMode::LOCAL_VAR:
            return "uint32_t";
        default:
            return "unknow type";
    }
}

String ASTUintType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return "uint32_t";
        case TypeMode::PARAM_IN:
            return "uint32_t";
        case TypeMode::PARAM_OUT:
            return "uint32_t&";
        case TypeMode::LOCAL_VAR:
            return "uint32_t";
        default:
            return "unknow type";
    }
}

String ASTUintType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    // unsupported type
    return "/";
}

void ASTUintType::EmitCWriteVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n",
        parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCProxyReadVar(const String& parcelName, const String& name, bool isInnerType,
    const String& ecName, const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCStubReadVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, %s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCppWriteVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCppReadVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s = 0;\n", EmitCppType().string(), name.string());
    }
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCMarshalling(const String& name, StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(data, %s)) {\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCUnMarshalling(const String& name, const String& gotoLabel, StringBuilder& sb,
    const String& prefix, std::vector<String>& freeObjStatements) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(data, &%s)) {\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    EmitFreeStatements(freeObjStatements, sb, prefix + g_tab);
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCppMarshalling(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}

void ASTUintType::EmitCppUnMarshalling(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, bool emitType, unsigned int innerLevel) const
{
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s = 0;\n", EmitCppType().string(), name.string());
    }
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%s)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
}
} // namespace HDI
} // namespace OHOS