#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# Copyright (c) 2021, Huawei Device Co., Ltd. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of Willow Garage, Inc. nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


import json
import os
import shutil
from string import Template

from hdf_tool_exception import HdfToolException
from .hdf_command_error_code import CommandErrorCode
from .hdf_command_handler_base import HdfCommandHandlerBase
from .hdf_defconfig_patch import HdfDefconfigAndPatch
from .hdf_device_info_hcs import HdfDeviceInfoHcsFile
from .hdf_vendor_build_file import HdfVendorBuildFile
from .hdf_vendor_kconfig_file import HdfVendorKconfigFile
from .hdf_vendor_makefile import HdfVendorMakeFile
from .hdf_vendor_mk_file import HdfVendorMkFile
from .hdf_module_kconfig_file import HdfModuleKconfigFile
from .hdf_module_mk_file import HdfModuleMkFile
from .hdf_driver_config_file import HdfDriverConfigFile
import hdf_utils


class HdfDeleteHandler(HdfCommandHandlerBase):
    def __init__(self, args):
        super(HdfDeleteHandler, self).__init__()
        self.cmd = 'delete'
        self.handlers = {
            'vendor': self._delete_vendor_handler,
            'module': self._delete_module_handler,
            'driver': self._delete_driver_handler,
        }
        self.parser.add_argument("--action_type",
                                 help=' '.join(self.handlers.keys()),
                                 required=True)
        self.parser.add_argument("--root_dir", required=True)
        self.parser.add_argument("--vendor_name")
        self.parser.add_argument("--module_name")
        self.parser.add_argument("--driver_name")
        self.parser.add_argument("--board_name")
        self.parser.add_argument("--kernel_name")
        self.args = self.parser.parse_args(args)

    def _delete_vendor_handler(self):
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("board_name")
        root, vendor, _, _, _ = self.get_args()
        vendor_hdf_dir = hdf_utils.get_vendor_hdf_dir(root, vendor)
        print(vendor_hdf_dir)
        if not os.path.exists(vendor_hdf_dir):
            return
        for module in os.listdir(vendor_hdf_dir):
            mod_dir = os.path.join(vendor_hdf_dir, module)
            if os.path.isdir(mod_dir):
                self._delete_module(root, module)
        shutil.rmtree(vendor_hdf_dir)

    def _delete_module(self, root, model, model_info):
        for key, path_value in model_info.items():
            if key.split("_")[-1] == "name":
                pass
            elif key == "driver_file_path":
                driver_file = os.path.join(
                    root, path_value.split(model)[0], model)
                if os.path.exists(driver_file):
                    shutil.rmtree(driver_file)
            else:
                self._delete_file_func(root, key, model_info, model)
        create_model_data = self._delete_model_info()
        delete_model_info = hdf_utils.get_create_model_info(
            root=root, create_data=json.dumps(create_model_data))
        return delete_model_info

    def _delete_model_info(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("module_name")
        root, _, module, _, _, _ = self.get_args()
        adapter_framework = hdf_utils.get_vendor_hdf_dir_framework(root=root)
        if not os.path.exists(adapter_framework):
            raise HdfToolException(
                ' adapter model path  "%s" not exist' %
                adapter_framework, CommandErrorCode.TARGET_NOT_EXIST)
        create_file_save_path = os.path.join(
            adapter_framework, "tools", "hdf_dev_eco_tool",
            "resources", "create_model.config")
        if not os.path.exists(create_file_save_path):
            raise HdfToolException(
                'create file config "%s" not exist' %
                create_file_save_path, CommandErrorCode.TARGET_NOT_EXIST)
        data = hdf_utils.read_file(create_file_save_path)
        write_data = json.loads(data)
        write_data.pop(module)
        hdf_utils.write_file(create_file_save_path,
                             json.dumps(write_data, indent=4))
        return write_data

    def _delete_module_handler(self):
        self.check_arg_raise_if_not_exist("root_dir")
        self.check_arg_raise_if_not_exist("module_name")
        root, _, module, _, _, _ = self.get_args()
        adapter_framework = hdf_utils.get_vendor_hdf_dir_framework(root=root)
        if not os.path.exists(adapter_framework):
            raise HdfToolException(
                ' adapter model path  "%s" not exist' %
                adapter_framework, CommandErrorCode.TARGET_NOT_EXIST)
        create_file_save_path = os.path.join(
            adapter_framework, "tools", "hdf_dev_eco_tool",
            "resources", "create_model.config")
        if not os.path.exists(create_file_save_path):
            raise HdfToolException(
                'create file config "%s" not exist' %
                create_file_save_path, CommandErrorCode.TARGET_NOT_EXIST)
        data = hdf_utils.read_file(create_file_save_path)
        file_info = json.loads(data)
        model_info = file_info.get(module, None)
        if model_info is None:
            raise HdfToolException(
                ' delete model "%s" not exist' %
                module, CommandErrorCode.TARGET_NOT_EXIST)
        else:
            return self._delete_module(root, module, model_info)

    def _delete_driver(self, module, driver):
        root, vendor, _, _, board = self.get_args()
        drv_dir = hdf_utils.get_drv_dir(root, vendor, module, driver)
        if os.path.exists(drv_dir):
            shutil.rmtree(drv_dir)
        k_path = hdf_utils.get_module_kconfig_path(root, vendor, module)
        HdfModuleKconfigFile(root, module, k_path).delete_driver(driver)
        HdfModuleMkFile(root, vendor, module).delete_driver(driver)
        HdfDriverConfigFile(root, board, module, driver).delete_driver()

    def _delete_driver_handler(self):
        self.check_arg_raise_if_not_exist("vendor_name")
        self.check_arg_raise_if_not_exist("module_name")
        self.check_arg_raise_if_not_exist("driver_name")
        self.check_arg_raise_if_not_exist("board_name")
        _, _, module, driver, _ = self.get_args()
        self._delete_driver(module, driver)

    def _delete_file_func(self, root, key, model_info, model):
        if key == "module_level_config_path":
            for key1, value1 in model_info[key].items():
                if key1 == "%s_Makefile" % model:
                    HdfVendorMakeFile(
                        root, vendor="", kernel="",
                        path=os.path.join(root, value1)).delete_module(model)
                elif key1 == "%s_Kconfig" % model:
                    HdfVendorKconfigFile(
                        root, vendor="", kernel="",
                        path=os.path.join(root, value1)).delete_module(model)
                elif key1 == "%sBuild" % model:
                    HdfVendorBuildFile(
                        root, vendor="").delete_module(
                        file_path=os.path.join(root, value1), model=model)
                elif key1 == "%s_hdf_lite" % model:
                    HdfVendorMkFile(
                        root, vendor="").delete_module(
                        file_path=os.path.join(root, value1), module=model)
                elif key1 == "%s_dot_configs" % model:
                    for dot_path in value1:
                        if dot_path.split(".")[-1] == "config":
                            template_string = \
                                "LOSCFG_DRIVERS_HDF_${module_upper_case}=y\n"
                        else:
                            template_string = \
                                "CONFIG_DRIVERS_HDF_${module_upper_case}=y\n"
                        new_demo_config = Template(template_string).\
                            substitute({"module_upper_case": model.upper()})
                        defconfig_patch = HdfDefconfigAndPatch(
                            root=root, vendor="", kernel="", board="",
                            data_model="", new_demo_config=new_demo_config)
                        defconfig_patch.delete_module(
                            path=os.path.join(root, dot_path))
        elif key == "module_path":
            for _, value2 in model_info[key].items():
                if value2.endswith("hcs"):
                    hcs_path = os.path.join(root, value2)
                    HdfDeviceInfoHcsFile(
                        root=root, vendor="", module="",
                        board="", driver="", path=hcs_path). \
                        delete_driver(module=model)
                else:
                    if value2:
                        file_path = os.path.join(root, value2)
                        if os.path.exists(file_path):
                            os.remove(file_path)
                        model_dir_path = "/".join(file_path.split("/")[:-1])
                        if os.path.exists(model_dir_path):
                            file_list = os.listdir(model_dir_path)
                            if not file_list:
                                shutil.rmtree(model_dir_path)