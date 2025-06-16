set_project("ProcessModifier")
set_version("1.0.0")

add_rules("mode.debug", "mode.release")

target("ProcessModifier")
    set_kind("binary")
    add_files("src/*.cpp")
    set_languages("c++17")

    -- Windows 平台特定配置
    if is_plat("windows") then
        add_syslinks("kernel32", "user32", "advapi32")
        add_defines("UNICODE", "_UNICODE")
        add_cxxflags("/EHsc", "/permissive-", "/Zc:__cplusplus", "/std:c++17")

        -- 设置管理员权限清单 (UAC)
        after_build(function(target)
            import("core.base.json")

            -- 1. 创建 UAC 清单文件
            local manifest_content = [[
    <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    <assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
        <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
            <security>
                <requestedPrivileges>
                    <requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>
                </requestedPrivileges>
            </security>
        </trustInfo>
        <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
            <application>
                <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/> <!-- Win10/Win11 -->
            </application>
        </compatibility>
    </assembly>
    ]]
            local manifest_path = path.join(os.tmpdir(), "ProcessModifier.manifest")
            io.writefile(manifest_path, manifest_content)

            -- 2. 获取输出文件路径
            local output_exe = path.join(target:targetdir(), "ProcessModifier.exe")

            -- 3. 使用 mt.exe 嵌入清单
            local mt_path = nil

            -- 尝试查找 mt.exe
            local candidates = {
                "C:/Program Files (x86)/Windows Kits/10/bin/**/x64/mt.exe",
                "C:/Program Files (x86)/Microsoft Visual Studio/**/VC/Tools/MSVC/**/bin/Hostx64/x64/mt.exe"
            }

            for _, pattern in ipairs(candidates) do
                local paths = os.files(pattern)
                if paths and #paths > 0 then
                    mt_path = paths[1]
                    break
                end
            end

            if mt_path then
                print("找到 mt.exe: " .. mt_path)

                -- 构建 mt 命令
                local command = string.format(
                    '"%s" -nologo -manifest "%s" -outputresource:"%s;#1"',
                    mt_path,
                    manifest_path,
                    output_exe
                )

                -- 执行命令
                os.exec(command)
                print("✅ UAC 清单嵌入成功")
            else
                print("⚠️ 警告：未找到 mt.exe，清单未嵌入")
                print("请安装 Windows SDK 或手动嵌入清单")
                print("临时解决方案：使用管理员权限运行程序")
            end

            

            -- 4. 清理临时文件
            os.tryrm(manifest_path)
        end)
    end

    -- 构建后操作：打包到 dist 目录
    after_build(function (target)
        import("core.project.config")

        -- 创建 dist 目录结构
        local dist_dir = path.join("$(projectdir)", "dist/")
        os.mkdir(dist_dir)

        -- 复制可执行文件
        os.cp(target:targetfile(), dist_dir)

        -- 复制配置文件
        os.cp("config/config.ini", dist_dir)

        -- 复制脚本文件
        os.cp("script/run_setup.ps1", dist_dir)
        os.cp("script/run.bat", dist_dir)
    end)


    -- 清理时删除 dist 目录
    on_clean(function ()
        os.rm("$(projectdir)/dist")
    end)
