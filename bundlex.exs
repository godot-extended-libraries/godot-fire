 defmodule Godot.BundlexProject do
   use Bundlex.Project

   def project() do
     [
       natives: natives(Bundlex.platform())
     ]
   end

   def natives(_platform) do
     [
       godot: [
         sources: ["godot.cpp"],
         includes: [".", "platform/server", "thirdparty/zlib", "main"],
         compiler_flags: ["-DSERVER_ENABLED", "-DUNIX_ENABLED", "-fpermissive", "-DTOOLS_ENABLED", "-Wno-unused-parameter"],
         interface: [:cnode],
         preprocessor: Unifex,
         lib_dirs: ["bin"],
         libs: ["godot_server.osx.tools.64"],
         language: :cpp
       ]
     ]
   end
 end
