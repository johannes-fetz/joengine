/*
** Jo Engine Cuesheet Maker
** Copyright (c) 2020-2026, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Creates game.cue from game.iso (data track) and the audio files of the current folder: *.wav, *.wave, *.mp3 and raw
** *.bin files become the audio tracks 2, 3, etc. in alphabetical order.
**
** v1.2: the IP.BIN written by the build (game.ip.bin, *.ip.bin) is not an audio track anymore (it shifted the tracks of
**       the music), the MP3 files get their TRACK line, the files are sorted (the order of the tracks no longer depends on
**       the file system)
*/

using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;

[assembly: AssemblyTitle("JoEngineCueMaker")]
[assembly: AssemblyDescription("Jo Engine Cuesheet Maker")]
[assembly: AssemblyCompany("jo-engine.org")]
[assembly: AssemblyProduct("JoEngineCueMaker")]
[assembly: AssemblyCopyright("Copyright 2020-2026 Johannes Fetz")]
[assembly: AssemblyVersion("1.2.0.0")]
[assembly: AssemblyFileVersion("1.2.0.0")]

namespace JoEngineCueMaker
{
    internal static class Program
    {
        private static void Main()
        {
            Console.BackgroundColor = ConsoleColor.Black;
            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine("Jo Engine Cuesheet Maker v1.2 (c) 2020-2026 by Johannes Fetz");
            Console.WriteLine();
            try
            {
                string cue = GenerateCue(Environment.CurrentDirectory);

                Console.WriteLine();
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0} generated.", Path.GetFileName(cue)));
            }
            catch (Exception ex)
            {
                Error(ex.Message);
            }
            Console.BackgroundColor = ConsoleColor.Black;
            Console.ForegroundColor = ConsoleColor.White;
        }

        private static string GenerateCue(string folderPath)
        {
            string isoFilePath = GetIsoFilePath(folderPath);
            int track = 1;
            StringBuilder cue = new StringBuilder();
            string[] files;

            Console.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0} found.", Path.GetFileName(isoFilePath)));
            GenerateCueHeader(cue, isoFilePath, ref track);
            files = Directory.GetFiles(folderPath, "*.*");
            // Alphabetical order: the order of the tracks
            Array.Sort(files, StringComparer.OrdinalIgnoreCase);
            foreach (string file in files)
                AppendFileToCue(cue, file, ref track);
            return ExportCueToFile(cue, folderPath, isoFilePath);
        }

        private static string GetIsoFilePath(string folderPath)
        {
            string[] isoFiles = Directory.GetFiles(folderPath, "*.iso");

            if (isoFiles == null || isoFiles.Length == 0)
                Error(string.Format(CultureInfo.InvariantCulture, "No ISO found in path: {0}", folderPath));
            if (isoFiles.Length > 1)
                Error(string.Format(CultureInfo.InvariantCulture, "Multiple ISO found in path: {0}", folderPath));
            return isoFiles[0];
        }

        private static void GenerateCueHeader(StringBuilder cue, string isoFilePath, ref int track)
        {
            cue.AppendLine(string.Format(CultureInfo.InvariantCulture, "FILE \"{0}\" BINARY", Path.GetFileName(isoFilePath)));
            cue.AppendLine(string.Format(CultureInfo.InvariantCulture, "  TRACK {0:D2} MODE1/2048", track++));
            cue.AppendLine("      INDEX 01 00:00:00");
            cue.AppendLine("      POSTGAP 00:02:00");
        }

        /// <summary>The IP.BIN of the build (game.ip.bin): the boot header of the ISO, not a track</summary>
        private static bool IsIpBin(string file)
        {
            return Path.GetFileName(file).ToLowerInvariant().EndsWith(".ip.bin", StringComparison.Ordinal);
        }

        private static void AppendFileToCue(StringBuilder cue, string file, ref int track)
        {
            string extension = Path.GetExtension(file).ToLowerInvariant();
            string type;

            if (extension == ".mp3")
                type = "MP3";
            else if (extension == ".bin" && !IsIpBin(file))
                type = "BINARY";
            else if (extension == ".wav" || extension == ".wave")
                type = "WAVE";
            else
                return;
            cue.AppendLine(string.Format(CultureInfo.InvariantCulture, "FILE \"{0}\" {1}", Path.GetFileName(file), type));
            Console.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0} found.", Path.GetFileName(file)));
            cue.AppendLine(string.Format(CultureInfo.InvariantCulture, "  TRACK {0:D2} AUDIO", track++));
            // (the gap after the data track: before the first audio track only)
            if (track == 3)
                cue.AppendLine("    PREGAP 00:02:00");
            cue.AppendLine("    INDEX 01 00:00:00");
        }

        private static string ExportCueToFile(StringBuilder cue, string folderPath, string isoFilePath)
        {
            string cueFilePath = Path.Combine(folderPath, Path.GetFileNameWithoutExtension(isoFilePath) + ".cue");

            File.WriteAllText(cueFilePath, cue.ToString());
            return cueFilePath;
        }

        private static void Error(string message)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine(message);
            Environment.Exit(-1);
        }
    }
}
