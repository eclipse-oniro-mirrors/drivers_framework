/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "ast/ast_list_type.h"
#include "util/options.h"

namespace OHOS {
namespace HDI {
bool ASTListType::IsListType()
{
    return true;
}

String ASTListType::ToString()
{
    return String::Format("List<%s>", elementType_->ToString().string());
}

TypeKind ASTListType::GetTypeKind()
{
    return TypeKind::TYPE_LIST;
}

String ASTListType::EmitCType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_IN: {
            if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
                return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
            }
            return String::Format("const %s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        }
        case TypeMode::PARAM_OUT:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        case TypeMode::LOCAL_VAR:
            return String::Format("%s*", elementType_->EmitCType(TypeMode::NO_MODE).string());
        default:
            return "unknow type";
    }
}

String ASTListType::EmitCppType(TypeMode mode) const
{
    switch (mode) {
        case TypeMode::NO_MODE:
            return String::Format("std::vector<%s>", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_IN:
            return String::Format("const std::vector<%s>&", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::PARAM_OUT:
            return String::Format("std::vector<%s>&", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        case TypeMode::LOCAL_VAR:
            return String::Format("std::vector<%s>", elementType_->EmitCppType(TypeMode::NO_MODE).string());
        default:
            return "unknow type";
    }
}

String ASTListType::EmitJavaType(TypeMode mode, bool isInnerType) const
{
    return String::Format("List<%s>", elementType_->EmitJavaType(mode, true).string());
}

void ASTListType::EmitCWriteVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n",
        parcelName.string(), lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    String elementName = "";
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT
        || elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        elementName = String::Format("&%s[i]", name.string());
    } else {
        elementName = String::Format("%s[i]", name.string());
    }

    elementType_->EmitCWriteVar(parcelName, elementName, ecName.string(), gotoLabel, sb, prefix + g_tab);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCProxyWriteOutVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String lenName = String::Format("*%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(%s, %s)) {\n",
        parcelName.string(), lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCProxyReadVar(const String& parcelName, const String& name, bool isInnerType,
    const String& ecName, const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, %s)) {\n",
        parcelName.string(), lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("for (i = 0; i < *%s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < *%s; i++) {\n", lenName.string());
    }

    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
        EmitCProxyReadStrElement(parcelName, name, ecName, gotoLabel, sb, prefix);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + g_tab);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        String element = String::Format("&%s[i]", name.string());
        String elementCp = String::Format("%sElementCp", name.string());
        elementType_->EmitCProxyReadVar(parcelName, elementCp, true, ecName, gotoLabel, sb, prefix + g_tab);
        sb.Append(prefix + g_tab).AppendFormat("(void)memcpy_s(%s, sizeof(%s), %s, sizeof(%s));\n",
            element.string(), elementType_->EmitCType().string(), elementCp.string(),
            elementType_->EmitCType().string());
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_FILEDESCRIPTOR) {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + g_tab);
    } else {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCProxyReadVar(parcelName, element, true, ecName, gotoLabel, sb, prefix + g_tab);
    }
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCStubReadVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String lenName = String::Format("%sLen", name.string());

    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, &%s)) {\n",
        parcelName.string(), lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n\n");

    sb.Append(prefix).AppendFormat("if (%s > 0) {\n", lenName.string());
    EmitCMallocVar(name, lenName, false, ecName, gotoLabel, sb, prefix + g_tab);
    sb.Append("\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + g_tab).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(prefix + g_tab).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
        EmitCStubReadStrElement(parcelName, name, ecName, gotoLabel, sb, prefix);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + g_tab + g_tab);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        String element = String::Format("%s[i]", name.string());
        String elementCp = String::Format("%sElementCp", name.string());
        elementType_->EmitCStubReadVar(parcelName, elementCp, ecName, gotoLabel, sb, prefix + g_tab + g_tab);
        sb.Append(prefix + g_tab + g_tab).AppendFormat("(void)memcpy_s(&%s, sizeof(%s), %s, sizeof(%s));\n",
            element.string(), elementType_->EmitCType().string(), elementCp.string(),
            elementType_->EmitCType().string());
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_FILEDESCRIPTOR) {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + g_tab + g_tab);
    } else {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, prefix + g_tab + g_tab);
    }
    sb.Append(prefix + g_tab).Append("}\n");
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCStubReadOutVar(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String lenName = String::Format("%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(%s, &%s)) {\n",
        parcelName.string(), lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s size failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n\n");

    sb.Append(prefix).AppendFormat("%s(%s, >, %s / sizeof(%s), %s, HDF_ERR_INVALID_PARAM, %s);\n",
        CHECK_VALUE_RET_GOTO_MACRO, lenName.string(), MAX_BUFF_SIZE_MACRO, elementType_->EmitCType().string(),
        ecName.string(), gotoLabel.string());

    sb.Append(prefix).AppendFormat("if (%s > 0) {\n", lenName.string());
    EmitCMallocVar(name, lenName, false, ecName, gotoLabel, sb, prefix + g_tab);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCppWriteVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s.size() failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n");
    String elementName = String::Format("it%d", innerLevel++);
    sb.Append(prefix).AppendFormat("for (auto %s : %s) {\n", elementName.string(), name.string());

    elementType_->EmitCppWriteVar(parcelName, elementName, sb, prefix + g_tab, innerLevel);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCppReadVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, bool initVariable, unsigned int innerLevel) const
{
    if (initVariable) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().string(), name.string());
    }
    sb.Append(prefix).AppendFormat("uint32_t %sSize = 0;\n", name.string());
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%sSize)) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).Append("HDF_LOGE(\"%{public}s: failed to read size\", __func__);\n");
    sb.Append(prefix + g_tab).Append("return HDF_ERR_INVALID_PARAM;\n");
    sb.Append(prefix).Append("}\n\n");

    sb.Append(prefix).AppendFormat("%s(%sSize, >, %s / sizeof(%s), HDF_ERR_INVALID_PARAM);\n",
        CHECK_VALUE_RETURN_MACRO, name.string(), MAX_BUFF_SIZE_MACRO, elementType_->EmitCppType().string());
    sb.Append(prefix).AppendFormat("for (uint32_t i%d = 0; i%d < %sSize; ++i%d) {\n",
        innerLevel, innerLevel, name.string(), innerLevel);
    String valueName = String::Format("value%d", innerLevel++);
    elementType_->EmitCppReadVar(parcelName, valueName, sb, prefix + g_tab, true, innerLevel);
    sb.Append(prefix + g_tab).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCMarshalling(const String& name, StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("if (!HdfSbufWriteUint32(data, %sLen)) {\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %sLen failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n");

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix).AppendFormat("for (i = 0; i < %sLen; i++) {\n", name.string());
    } else {
        sb.Append(prefix).AppendFormat("for (uint32_t i = 0; i < %sLen; i++) {\n", name.string());
    }

    String elementName = String::Format("(%s)[i]", name.string());
    elementType_->EmitCMarshalling(elementName, sb, prefix + g_tab);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCUnMarshalling(const String& name, const String& gotoLabel, StringBuilder& sb,
    const String& prefix, std::vector<String>& freeObjStatements) const
{
    String lenName = String::Format("%sLen", name.string());
    sb.Append(prefix).AppendFormat("if (!HdfSbufReadUint32(data, &%s)) {\n", lenName.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n", lenName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");

    sb.Append(prefix).AppendFormat("if (%s > %s / sizeof(%s)) {\n", lenName.string(), MAX_BUFF_SIZE_MACRO,
        elementType_->EmitCType().string());
    sb.Append(prefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: %s is invalid data\", __func__);\n",
        lenName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");

    sb.Append(prefix).AppendFormat("if (%s > 0) {\n", lenName.string());
    String newPrefix = prefix + g_tab;

    sb.Append(newPrefix).AppendFormat("%s = (%s*)OsalMemCalloc(sizeof(%s) * %s);\n",
        name.string(), elementType_->EmitCType().string(), elementType_->EmitCType().string(), lenName.string());
    sb.Append(newPrefix).AppendFormat("if (%s == NULL) {\n", name.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(newPrefix).Append("}\n");

    freeObjStatements.push_back(String::Format("OsalMemFree(%s);\n", name.string()));

    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(newPrefix).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
    } else {
        sb.Append(newPrefix).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
    }

    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING) {
        EmitCStringElementUnMarshalling(name, gotoLabel, sb, newPrefix, freeObjStatements);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        String element = String::Format("&%s[i]", name.string());
        elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix + g_tab, freeObjStatements);
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        String element = String::Format("%s[i]", name.string());
        String elementCp = String::Format("%sElementCp", name.string());
        elementType_->EmitCUnMarshalling(elementCp, gotoLabel, sb, newPrefix + g_tab, freeObjStatements);
        sb.Append(newPrefix + g_tab).AppendFormat("(void)memcpy_s(&%s, sizeof(%s), %s, sizeof(%s));\n",
            element.string(), elementType_->EmitCType().string(), elementCp.string(),
            elementType_->EmitCType().string());
    } else {
        String element = String::Format("%s[i]", name.string());
        elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix + g_tab, freeObjStatements);
    }
    sb.Append(newPrefix).Append("}\n");
    sb.Append(prefix).Append("}\n");
    freeObjStatements.pop_back();
}

void ASTListType::EmitCStringElementUnMarshalling(const String& name, const String& gotoLabel, StringBuilder& sb,
    const String& newPrefix, std::vector<String>& freeObjStatements) const
{
    String element = String::Format("%sElement", name.string());
    elementType_->EmitCUnMarshalling(element, gotoLabel, sb, newPrefix + g_tab, freeObjStatements);
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(newPrefix).AppendFormat("%s[i] = (char*)OsalMemCalloc(strlen(%s) + 1);\n",
            name.string(), element.string());
        sb.Append(newPrefix).AppendFormat("if (%s[i] == NULL) {\n", name.string());
        sb.Append(newPrefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n\n");
        sb.Append(newPrefix).AppendFormat("if (strcpy_s((%s)[i], (strlen(%s) + 1), %s) != HDF_SUCCESS) {\n",
            name.string(), element.string(), element.string());
        sb.Append(newPrefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n",
            element.string());
        sb.Append(newPrefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n");
    } else {
        sb.Append(newPrefix).Append(g_tab).AppendFormat("%s[i] = strdup(%s);\n",
            name.string(), element.string());
    }
}

void ASTListType::EmitCppMarshalling(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, unsigned int innerLevel) const
{
    sb.Append(prefix).AppendFormat("if (!%s.WriteUint32(%s.size())) {\n", parcelName.string(), name.string());
    sb.Append(prefix + g_tab).AppendFormat(
        "HDF_LOGE(\"%%{public}s: write %s.size failed!\", __func__);\n", name.string());
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n");
    String elementName = String::Format("it%d", innerLevel++);
    sb.Append(prefix).AppendFormat("for (auto %s : %s) {\n", elementName.string(), name.string());

    elementType_->EmitCppMarshalling(parcelName, elementName, sb, prefix + g_tab, innerLevel);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCppUnMarshalling(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix, bool emitType, unsigned int innerLevel) const
{
    int index = name.IndexOf('.', 0);
    String memberName = name.Substring(index + 1);
    String sizeName = String::Format("%sSize", memberName.string());
    if (emitType) {
        sb.Append(prefix).AppendFormat("%s %s;\n", EmitCppType().string(), memberName.string());
    }

    sb.Append(prefix).AppendFormat("uint32_t %s = 0;\n", sizeName.string());
    sb.Append(prefix).AppendFormat("if (!%s.ReadUint32(%s)) {\n", parcelName.string(), sizeName.string());
    sb.Append(prefix + g_tab).Append("HDF_LOGE(\"%{public}s: failed to read size\", __func__);\n");
    sb.Append(prefix + g_tab).Append("return false;\n");
    sb.Append(prefix).Append("}\n\n");

    sb.Append(prefix).AppendFormat("%s(%s, >, %s / sizeof(%s), false);\n",
        CHECK_VALUE_RETURN_MACRO, sizeName.string(), MAX_BUFF_SIZE_MACRO, elementType_->EmitCppType().string());
    sb.Append(prefix).AppendFormat("for (uint32_t i%d = 0; i%d < %s; ++i%d) {\n",
        innerLevel, innerLevel, sizeName.string(), innerLevel);

    String valueName = String::Format("value%d", innerLevel++);
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        sb.Append(prefix + g_tab).AppendFormat("%s %s;\n",
            elementType_->EmitCppType().string(), valueName.string());
        elementType_->EmitCppUnMarshalling(parcelName, valueName, sb, prefix + g_tab, true, innerLevel);
        sb.Append(prefix + g_tab).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());
    } else if (elementType_->GetTypeKind() == TypeKind::TYPE_UNION) {
        sb.Append(prefix + g_tab).AppendFormat("%s %s;\n",
            elementType_->EmitCppType().string(), valueName.string());
        String cpName = String::Format("%sCp", valueName.string());
        elementType_->EmitCppUnMarshalling(parcelName, cpName, sb, prefix + g_tab, true, innerLevel);
        sb.Append(prefix + g_tab).AppendFormat("(void)memcpy_s(&%s, sizeof(%s), %s, sizeof(%s));\n",
            valueName.string(), elementType_->EmitCppType().string(), cpName.string(),
            elementType_->EmitCppType().string());
        sb.Append(prefix + g_tab).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());
    } else {
        elementType_->EmitCppUnMarshalling(parcelName, valueName, sb, prefix + g_tab, true, innerLevel);
        sb.Append(prefix + g_tab).AppendFormat("%s.push_back(%s);\n", name.string(), valueName.string());
    }
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitMemoryRecycle(const String& name, bool isClient, bool ownership, StringBuilder& sb,
    const String& prefix) const
{
    String varName = name;
    String lenName = isClient ? String::Format("*%sLen", name.string()) : String::Format("%sLen", name.string());

    sb.Append(prefix).AppendFormat("if (%s > 0 && %s != NULL) {\n", lenName.string(), varName.string());
    if (elementType_->GetTypeKind() == TypeKind::TYPE_STRING || elementType_->GetTypeKind() == TypeKind::TYPE_STRUCT) {
        if (Options::GetInstance().DoGenerateKernelCode()) {
            sb.Append(prefix + g_tab).AppendFormat("for (i = 0; i < %s; i++) {\n", lenName.string());
        } else {
            sb.Append(prefix + g_tab).AppendFormat("for (uint32_t i = 0; i < %s; i++) {\n", lenName.string());
        }

        String elementName = String::Format("%s[i]", varName.string());
        elementType_->EmitMemoryRecycle(elementName, false, false, sb, prefix + g_tab + g_tab);
        sb.Append(prefix + g_tab).Append("}\n");
    }

    sb.Append(prefix + g_tab).AppendFormat("OsalMemFree(%s);\n", varName.string());
    if (isClient) {
        sb.Append(prefix + g_tab).AppendFormat("%s = NULL;\n", varName.string());
    }

    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitJavaWriteVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix) const
{
    sb.Append(prefix).AppendFormat("%s.writeInt(%s.size());\n", parcelName.string(), name.string());
    sb.Append(prefix).AppendFormat("for (%s element : %s) {\n",
        elementType_->EmitJavaType(TypeMode::NO_MODE).string(), name.string());
    elementType_->EmitJavaWriteVar(parcelName, "element", sb, prefix + g_tab);
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitJavaReadVar(const String& parcelName, const String& name, StringBuilder& sb,
    const String& prefix) const
{
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.string());

    elementType_->EmitJavaReadInnerVar(parcelName, "value", false, sb, prefix + g_tab);
    sb.Append(prefix + g_tab).AppendFormat("%s.add(value);\n", name.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitJavaReadInnerVar(const String& parcelName, const String& name, bool isInner,
    StringBuilder& sb, const String& prefix) const
{
    sb.Append(prefix).AppendFormat("%s %s = new Array%s();\n",
        EmitJavaType(TypeMode::NO_MODE).string(), name.string(), EmitJavaType(TypeMode::NO_MODE).string());
    sb.Append(prefix).AppendFormat("int %sSize = %s.readInt();\n", name.string(), parcelName.string());
    sb.Append(prefix).AppendFormat("for (int i = 0; i < %sSize; ++i) {\n", name.string());
    elementType_->EmitJavaReadInnerVar(parcelName, "value", true, sb, prefix + g_tab);
    sb.Append(prefix + g_tab).AppendFormat("%s.add(value);\n", name.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCMallocVar(const String& name, const String& lenName, bool isClient, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String varName = isClient ? String::Format("*%s", name.string()) : name;
    String lenVarName = isClient ? String::Format("*%s", lenName.string()) : lenName;

    sb.Append(prefix).AppendFormat("%s = (%s*)OsalMemCalloc(sizeof(%s) * (%s));\n", varName.string(),
        elementType_->EmitCType().string(), elementType_->EmitCType().string(), lenVarName.string());
    sb.Append(prefix).AppendFormat("if (%s == NULL) {\n", varName.string());
    sb.Append(prefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s failed\", __func__);\n",
        varName.string());
    sb.Append(prefix + g_tab).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.string());
    sb.Append(prefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
    sb.Append(prefix).Append("}\n");
}

void ASTListType::EmitCProxyReadStrElement(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String cpName = String::Format("%sCp", name.string());
        elementType_->EmitCProxyReadVar(parcelName, cpName, true, ecName, gotoLabel, sb, prefix + g_tab);
    sb.Append("\n");
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(prefix + g_tab).AppendFormat("%s[i] = (char*)OsalMemCalloc(strlen(%s) + 1);\n",
            name.string(), cpName.string());
        sb.Append(prefix + g_tab).AppendFormat("if (%s[i] == NULL) {\n", name.string());
        sb.Append(prefix + g_tab + g_tab).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.string());
        sb.Append(prefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: malloc %s[i] failed\", __func__);\n",
            name.string());
        sb.Append(prefix + g_tab + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix + g_tab).Append("}\n\n");
        sb.Append(prefix + g_tab).AppendFormat("if (strcpy_s(%s[i], (strlen(%s) + 1), %s) != HDF_SUCCESS) {\n",
            name.string(), cpName.string(), cpName.string());
        sb.Append(prefix + g_tab + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n",
            cpName.string());
        sb.Append(prefix + g_tab + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(prefix + g_tab + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(prefix + g_tab).Append("}\n");
    } else {
        sb.Append(prefix + g_tab).AppendFormat("%s[i] = strdup(%sCp);\n",
            name.string(), name.string());
    }
}

void ASTListType::EmitCStubReadStrElement(const String& parcelName, const String& name, const String& ecName,
    const String& gotoLabel, StringBuilder& sb, const String& prefix) const
{
    String element = String::Format("%sCp", name.string());
    String newPrefix = prefix + g_tab + g_tab;
    elementType_->EmitCStubReadVar(parcelName, element, ecName, gotoLabel, sb, newPrefix);
    sb.Append("\n");
    if (Options::GetInstance().DoGenerateKernelCode()) {
        sb.Append(newPrefix).AppendFormat("%s[i] = (char*)OsalMemCalloc(strlen(%s) + 1);\n",
            name.string(), element.string());
        sb.Append(newPrefix).AppendFormat("if (%s[i] == NULL) {\n", name.string());
        sb.Append(newPrefix + g_tab).AppendFormat("%s = HDF_ERR_MALLOC_FAIL;\n", ecName.string());
        sb.Append(newPrefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n\n");

        sb.Append(newPrefix).AppendFormat("if (strcpy_s(%s[i], (strlen(%s) + 1), %s) != HDF_SUCCESS) {\n",
            name.string(), element.string(), element.string());
        sb.Append(newPrefix + g_tab).AppendFormat("HDF_LOGE(\"%%{public}s: read %s failed!\", __func__);\n",
            element.string());
        sb.Append(newPrefix + g_tab).AppendFormat("%s = HDF_ERR_INVALID_PARAM;\n", ecName.string());
        sb.Append(newPrefix + g_tab).AppendFormat("goto %s;\n", gotoLabel.string());
        sb.Append(newPrefix).Append("}\n");
    } else {
        sb.Append(newPrefix).AppendFormat("%s[i] = strdup(%sCp);\n", name.string(), name.string());
    }
}
} // namespace HDI
} // namespace OHOS