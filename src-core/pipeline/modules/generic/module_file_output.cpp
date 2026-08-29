#include "module_file_output.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "utils/file.h"
#include <cmath>

namespace satdump
{
    namespace pipeline
    {
        namespace generic
        {
            std::string FileOutputModule::getID() { return "file_output"; }

            FileOutputModule::FileOutputModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : ProcessingModule(input_file, output_file_hint, parameters)
            {
                if (parameters.contains("extension"))
                    file_ext = parameters["extension"].get<std::string>();
            }

            void FileOutputModule::init()
            {
                // Init input
                filesize = input_data_type == DATA_FILE ? getFilesize(d_input_file) : 0;
                if (input_data_type == DATA_FILE)
                {
                    data_in = std::ifstream(d_input_file, std::ios::binary);
                    logger->info("Using input file " + d_input_file);
                }
                progress = 0;

                d_output_file = d_output_file_hint + file_ext;



                // If the input is already the file we would write to, there is nothing
                // to do: opening it for output would truncate our own input!
                if (d_input_file == d_output_file)
                {
                    logger->info("Input is already the output file, nothing to do (" + d_output_file + ")");
                    return;
                }

                if (output_data_type == DATA_FILE)
                {
                    data_out = std::ofstream(d_output_file, std::ios::binary);
                    logger->info("Saving output to " + d_output_file_hint + file_ext);
                }

            }

            void FileOutputModule::process()
            {
                if (d_input_file == d_output_file)
                {
                    progress = filesize;
                    return;
                }

                uint8_t buffer[8192];

                while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
                {
                    // Read buffer
                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)buffer, sizeof(buffer));
                    else
                        input_fifo->read(buffer, sizeof(buffer));

                    size_t read_size = input_data_type == DATA_FILE ? data_in.gcount() : sizeof(buffer);

                    if (read_size == 0)
                        continue;

                    // Write buffer
                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)buffer, read_size);
                    else
                        output_fifo->write(buffer, read_size);

                    if (input_data_type == DATA_FILE)
                        progress = data_in.tellg();

                    // Log progress occasionally
                    if (time(NULL) != last_log_time)
                    {
                        last_log_time = time(NULL);
                        if (!d_is_streaming_input)
                        {
                            double pct = filesize > 0 ? ((double)progress / (double)filesize) * 100.0 : 0.0;
                            logger->info("Progress " + std::to_string(round(pct * 10.0) / 10.0) + "%%");
                        }
                    }
                }

                if (input_data_type == DATA_FILE)
                    data_in.close();
                if (output_data_type == DATA_FILE)
                    data_out.close();
            }

            void FileOutputModule::stop()
            {
                input_active = false;
                if (input_data_type == DATA_FILE && data_in.is_open())
                    data_in.close();
                if (output_data_type == DATA_FILE && data_out.is_open())
                    data_out.close();
            }

            void FileOutputModule::drawUI(bool window)
            {
                ImGui::Begin("File Output", NULL, window ? 0 : NOWINDOW_FLAGS);
                ImGui::Text("Output: ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.green, "%s", d_output_file.c_str());

                if (!d_is_streaming_input)
                    ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

                ImGui::End();
            }

            nlohmann::json FileOutputModule::getModuleStats()
            {
                nlohmann::json v;
                if (!d_is_streaming_input && filesize > 0)
                    v["progress"] = ((double)progress / (double)filesize) * 100.0;
                return v;
            }

            nlohmann::json FileOutputModule::getParams()
            {
                return nlohmann::json{
                    {"extension", {{"type", "string"}, {"default", ".ts"}, {"description", "File extension for output file"}}}};
            }
        } // namespace generic
    } // namespace pipeline
} // namespace satdump