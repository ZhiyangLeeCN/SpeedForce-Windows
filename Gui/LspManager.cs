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

        [DllImport(LSP_MANAGER_DLL, EntryPoint = "VerifySfLspIsInstalled")]
        private static extern int _verifySfLspIsInstalled();

        [DllImport(LSP_MANAGER_DLL, EntryPoint = "UninstallSfLsp")]
        private static extern int _uninstallSfLsp();

        [DllImport(LSP_MANAGER_DLL, CharSet = CharSet.Unicode, EntryPoint = "InstallSfLsp")]
        private static extern int _installSfLsp(String lspPathAndFile, String lspPathAndFile32);

        public static int VerifySfLspIsInstalled()
        {
            return _verifySfLspIsInstalled();
        }

        public static int UninstallSfLsp()
        {
            return _uninstallSfLsp();
        }

        public static int InstallSfLsp(String lspPathAndFile, String lspPathAndFile32)
        {
            return _installSfLsp(lspPathAndFile, lspPathAndFile32);
        }
    }
}
