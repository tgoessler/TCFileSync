//*******************************************************************************
//
// *******   ***   ***               *
//    *     *     *                  *
//    *    *      *                *****
//    *    *       ***  *   *   **   *    **    ***
//    *    *          *  * *   *     *   ****  * * *
//    *     *         *   *      *   * * *     * * *
//    *      ***   ***    *     **   **   **   *   *
//                        *
//*******************************************************************************
// see http://sourceforge.net/projects/tcsystem/ for details.
// Copyright (C) 2003 - 2021 Thomas Goessler. All Rights Reserved. 
//*******************************************************************************

#include "TCApplication.h"
#include "TCFileName.h"
#include "TCFileSyncSyncronizer.h"
#include "TCFileSyncVersion.h"
#include "TCOutput.h"
#include "TCString.h"
#include "TCWString.h"
#include "TCWFile.h"
#include "TCWFileName.h"
#include "TCSystem.h"

#include "TCNewEnable.h"

namespace tc::file_sync
{
   static StreamPtr s_out;

   class MTTraceTarget : public output::PrintTarget
   {
   public:
      MTTraceTarget()
         = default;

      void Print(const char* text) override
      {
         s_out << text << endl << flush;
      }
   };

   class FileSyncApplication final : public Application
   {
   public:
      FileSyncApplication()
      {
         s_out = factory::CreateStdOutStream();

         const output::PrintTargetPtr trace_target(new MTTraceTarget);
         output::SetErrorTarget(trace_target);
         output::SetWarningTarget(trace_target);
         output::SetInfoTarget(trace_target);
         output::SetTraceTarget(trace_target);

         s_out << PROGRAM_NAME ": Version " PROGRAM_VERSION_STR << endl
            << "            This product includes: " << TCPRODUCT_STR "-" TCVERSION_STR
            << endl << endl;

         m_settings.info_mode = false;
         m_settings.calc_checksum = false;
         m_settings.backup_folder = L"_Older";
         m_settings.num_backups = 5;
      }

      ~FileSyncApplication() override
      {
         output::SetErrorTarget(output::PrintTargetPtr());
         output::SetWarningTarget(output::PrintTargetPtr());
         output::SetInfoTarget(output::PrintTargetPtr());
         output::SetTraceTarget(output::PrintTargetPtr());

         s_out = StreamPtr();
      }

      [[nodiscard]] bool Run() const
      {
         Syncronizer synchronizer(m_settings, StatusDisplayerPtr());
         if (!synchronizer.SetupSyncronisationData())
         {
            TCERRORS("FileSync", "Failed setting up synchronization data");
            return false;
         }
         if (!synchronizer.SyncDestination())
         {
            TCERRORS("FileSync", "Failed syncing directories");
            return false;
         }

         return true;
      }

   protected:
      bool ProcessArguments(const std::vector< std::string >& argv) override
      {
         for (auto it = argv.begin(); it != argv.end(); ++it)
         {
            if (*it == "--source" || *it == "-s")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.source = wfile_name::Simplify(val);
            }
            else if (*it == "--destination" || *it == "-d")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.destination = wfile_name::Simplify(val);
               if (!wfile::Exists(m_settings.destination))
               {
                  wfile::CreateDir(m_settings.destination);
               }
            }
            else if (*it == "--backup_folder" || *it == "-b")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.backup_folder = wfile_name::Simplify(val);
            }
            else if (*it == "--num_backups" || *it == "-n")
            {
               ++it;
               m_settings.num_backups = string::ToUint32(*it);
            }
            else if (*it == "--skipp")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.files_and_folders_to_skipp.insert(val);
            }
            else if (*it == "--skipp_ext")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.extensions_to_skipp.insert(val);
            }
            else if (*it == "--ignore_dest")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.destination_files_and_folders_to_ignore.insert(val);
            }
            else if (*it == "--ext")
            {
               auto val = wstring::ToString(*(++it));
               m_settings.extensions_to_search_for.insert(val);
            }
            else if (*it == "--info_only" || *it == "-i")
            {
               m_settings.info_mode = true;
            }
            else if (*it == "--calc_checksum" || *it == "-c")
            {
               m_settings.calc_checksum = true;
            }
            else if (*it == "--empty_directories")
            {
               m_settings.create_directories = true;
            }
            else
            {
               DisplayUsage();
               return false;
            }
         }

         if ((m_settings.source.empty() || m_settings.destination.empty()))
         {
            DisplayUsage();
            return false;
         }

         return true;

      }

      std::string GetUsage() override
      {
         return
            "Usage: -s or --source        for source directory\n"
            "       -d or --destination   for destination directory\n"
            "       -b or --backup_folder folder into which to make backups of removed or modified files\n"
            "                             default = _Older\n"
            "       -n or --num_backups   number of backups to keep for one file\n"
            "                             default = 5\n"
            "       --ext                 use only files with specified extension during synchronization, can be set more than once\n"
            "       --skip                folder name to skip during synchronization, can be set more than once\n"
            "       --skip_ext            skip file names with specified extension during synchronization, can be set more than once\n"
            "       --ignore_dest         do not delete existing destination files or directories\n"
            "       --empty_directories   create also empty directories\n";
      }

   private:
      Settings m_settings;
   };

}

int main(int narg, char** argv)
{
   {
      const std::shared_ptr<tc::file_sync::FileSyncApplication> app(new tc::file_sync::FileSyncApplication);
      if (!app->Init(narg, argv, PROGRAM_NAME, PROGRAM_VERSION_STR, PROGRAM_COMPANY))
      {
         return 1;
      }

      if (!app->Run())
      {
         return 2;
      }
   }

   return 0;
}

