using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Gui
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            MessageBox.Show(LspManager.VerifySfLspIsInstalled().ToString());
        }

        private void button2_Click(object sender, EventArgs e)
        {
            MessageBox.Show(LspManager.UninstallSfLsp().ToString());
        }

        private void button3_Click(object sender, EventArgs e)
        {
            MessageBox.Show(
                LspManager.InstallSfLsp().ToString()
                );
        }
    }
}
