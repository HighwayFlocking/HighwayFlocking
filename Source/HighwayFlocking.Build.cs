/* Copyright 2015 Sindre Ilebekk Johansen and Andreas Sløgedal Løvland

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class HighwayFlocking : ModuleRules
	{
        private string ModulePath
        {
            get { return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name)); }
        }

        public HighwayFlocking(TargetInfo Target)
		{
            // Run the git_hash.py script in the Source folder

            string strCmdText;
            strCmdText = "\"" +  Path.Combine(ModulePath, "git_hash.py") + "\"";

            var process = new System.Diagnostics.Process
            {
                StartInfo = new System.Diagnostics.ProcessStartInfo
                {
                    FileName = "python.exe",
                    Arguments = strCmdText
                }
            };
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.RedirectStandardOutput = true;
            process.OutputDataReceived += (sender, args) => Console.WriteLine("{0}", args.Data);
            process.Start();
            process.BeginOutputReadLine();
            process.WaitForExit();

			PublicIncludePaths.AddRange(
				new string[] {
                    "Landscape",
                    "Sockets",
                    "Networking",
                    "UMG",
					// ... add public include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "InputCore",
                    "LandScape",
                    "Sockets",
                    "Networking",
                    "UMG",
                    "PhysX", "APEX" //PhysX
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "RenderCore",
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}
