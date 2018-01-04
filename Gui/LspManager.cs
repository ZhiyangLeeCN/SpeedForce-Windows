using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Gui
{
    class LspManager
    {
        private const String LSP_MANAGER_DLL = "lsp_manager.dll";

        [DllImport(LSP_MANAGER_DLL, EntryPoint = "VerifyLspIsInstalled")]
        private static extern int _verifyLspIsInstalled();

        public static int VerifyLspIsInstalled()
        {
            return _verifyLspIsInstalled();
        }
    }
}
