function Torque6Editor()
    project "Torque6Editor"
        targetname "Torque6Editor"
        language "C++"
        kind "WindowedApp"
        debugdir (EDITOR_DIR)

        includedirs {
            SRC_DIR,
            EDITOR_DIR,
            path.join(LIB_DIR, "assimp/include"),
            path.join(LIB_DIR, "bgfx/include"),
            path.join(LIB_DIR, "bgfx/common"),
            path.join(LIB_DIR, "openal/win32"),
        }

        files {
            path.join(EDITOR_DIR, "src/**.cpp"),
            path.join(EDITOR_DIR, "src/**.h"),
        }

        links {
            "Torque6"
        }

        defines {
            "TORQUE_PLUGIN"
        }

        configuration "vs2015"
            windowstargetplatformversion "10.0.10240.0"

        configuration "Debug"
            targetname "Torque6Editor_DEBUG"
            defines     { "TORQUE_DEBUG", "TORQUE_ENABLE_PROFILER" }
            flags       { "Symbols" }

        configuration "Release"
            defines     {  }

        configuration "vs*"
            defines     { "_CRT_SECURE_NO_WARNINGS" }

        configuration "windows"
            flags { "WinMain" }
            targetdir   "../bin/windows"
            links { "ole32",
                    "winmm",
                    "comctl32",
                    "rpcrt4",
                    "wsock32",
                    "wininet", 
                  }
            includedirs { "$(wxwin)/include",
                          "$(wxwin)/src/stc/scintilla/include",
                          "$(wxwin)/src/stc/scintilla/lexlib",
                          "$(wxwin)/src/stc/scintilla/src",
                        }
            
        configuration { "windows", "x32", "Release" }
            includedirs { "$(wxwin)/lib/vc_lib/mswu" }
            libdirs { "$(wxwin)/lib/vc_lib" }

        configuration { "windows", "x32", "Debug" }
            includedirs { "$(wxwin)/lib/vc_lib/mswu" }
            libdirs { "$(wxwin)/lib/vc_lib" }

        configuration { "windows", "x64", "Release" }
            includedirs { "$(wxwin)/lib/vc_lib/mswu" }
            libdirs { "$(wxwin)/lib/vc_x64_lib" }
            targetdir   "../bin/windows.x64"

        configuration { "windows", "x64", "Debug" }
            includedirs { "$(wxwin)/lib/vc_lib/mswud" }
            libdirs { "$(wxwin)/lib/vc_x64_lib" }
            targetdir   "../bin/windows.x64"

        configuration { "windows", "Release" }
            links { "wxmsw31u_adv",
                    "wxmsw31u_aui",
                    "wxmsw31u_core",
                    "wxmsw31u_propgrid",
                    "wxmsw31u_stc",
                    "wxbase31u",
                    "wxbase31u_net",
                    "wxscintilla",
                    "wxtiff",
                    "wxjpeg",
                    "wxpng",
                    "wxzlib",
                    "wxregexu",
                    "wxexpat",
                  }

        configuration { "windows", "Debug" }
            links { "wxmsw31ud_adv",
                    "wxmsw31ud_aui",
                    "wxmsw31ud_core",
                    "wxmsw31ud_propgrid",
                    "wxmsw31ud_stc",
                    "wxbase31ud",
                    "wxbase31ud_net",
                    "wxscintillad",
                    "wxtiffd",
                    "wxjpegd",
                    "wxpngd",
                    "wxzlibd",
                    "wxregexud",
                    "wxexpatd",
                  }

        configuration "linux"
            targetdir   "../bin/linux"
            links       { "dl" }
            linkoptions { "-rdynamic" }

        configuration "bsd"
            targetdir   "../bin/bsd"

        configuration "linux or bsd"
            defines     {  }
            links       { "m" }
            linkoptions { "-rdynamic" }

        configuration "macosx"
            targetdir   "../bin/darwin"
            defines     {  }
            links       { "CoreServices.framework" }

        configuration { "macosx", "gmake" }
            buildoptions { "-mmacosx-version-min=10.4" }
            linkoptions  { "-mmacosx-version-min=10.4" }
end
