#pragma once

#include "pipeline/module.h"
#include <ctime>
#include <fstream>

namespace satdump
{
    namespace pipeline
    {
        namespace generic
        {
            class FileOutputModule : public ProcessingModule
            {
            public:
                FileOutputModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~FileOutputModule() {}

                std::vector<ModuleDataType> getInputTypes() override { return {DATA_FILE, DATA_STREAM}; }
                std::vector<ModuleDataType> getOutputTypes() override { return {DATA_FILE}; }

                void init() override;
                void process() override;
                void stop() override;
                void drawUI(bool window) override;
                nlohmann::json getModuleStats() override;

                static std::string getID();
                virtual std::string getIDM() { return getID(); }

                static nlohmann::json getParams();
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                {
                    return std::make_shared<FileOutputModule>(input_file, output_file_hint, parameters);
                }

            private:
                std::ifstream data_in;
                std::ofstream data_out;
                std::string file_ext = ".ts";

                uint64_t filesize = 0;
                uint64_t progress = 0;
                time_t last_log_time = 0;
            };
        } // namespace generic
    } // namespace pipeline
} // namespace satdump