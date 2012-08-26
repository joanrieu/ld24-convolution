solution "ld24"
        configurations { "Debug", "Release" }
        configuration "Debug"
                flags "Symbols"
        configuration "Release"
                flags "Optimize"
        project "convolution"
                kind "WindowedApp"
                language "C++"
                files "**.cpp"
                links { "sfml-system", "sfml-window", "sfml-graphics", "sfml-audio" }
