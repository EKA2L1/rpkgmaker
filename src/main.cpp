#include <cstdint>
#include <filesystem>
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct RPKGHeader {
    std::uint32_t magic[4];
    std::uint32_t rom_version;
    std::uint32_t file_count;
    std::uint32_t header_size;
    std::uint32_t machine_uid;
};

struct RPKGEntry {
    std::uint64_t attrib;
    std::uint64_t last_modified;
    std::uint64_t full_path_length;
};
#pragma pack(pop)

int main(const int argc, char *argv[]) {
    std::cout << "rpkgmaker [folder_path] [machine_uid]" << std::endl;

    if (argc <= 1) {
        return 0;
    } 

    const char *folder_path = argv[1];
    const std::uint32_t machine_uid = std::atoi(argv[2]);

    std::ofstream fout("SYM.RPKG", std::ios_base::binary);
    RPKGHeader header;
    header.magic[0] = 'R';
    header.magic[1] = 'P';
    header.magic[2] = 'K';
    header.magic[3] = '2';
    header.rom_version = 0;
    header.file_count = 0;
    header.header_size = sizeof(RPKGHeader);
    header.machine_uid = machine_uid;

    fs::path final_path = fs::absolute(folder_path);
    std::string final_path_str = final_path.string();

    fout.write(reinterpret_cast<const char*>(&header), header.header_size);
    for (auto &entry: fs::recursive_directory_iterator(final_path)) {
        if (entry.is_regular_file()) {
            RPKGEntry rentry;

            // This includes read-only and archive attribute
            rentry.attrib = 1 | 0x20;
            rentry.last_modified = std::chrono::duration_cast<std::chrono::microseconds>(entry.last_write_time().
                time_since_epoch()).count() + 62167132800 * 1000000;

            std::string entry_path_str = entry.path().string();
            entry_path_str.erase(entry_path_str.begin(), entry_path_str.begin() + final_path_str.size());

            for (auto i = 0; i < entry_path_str.size(); i++) {
                if (entry_path_str[i] == '/') {
                    entry_path_str[i] = '\\';
                }
            }

            if (entry_path_str.empty() || (entry_path_str[0] != '\\')) {
                entry_path_str.insert(entry_path_str.begin(), '\\');
            }

            entry_path_str = "Z:" + entry_path_str;
            rentry.full_path_length = entry_path_str.length();

            std::uint64_t rentry_fsize = entry.file_size();

            fout.write(reinterpret_cast<const char*>(&rentry), sizeof(RPKGEntry));
            for (std::size_t i = 0; i < entry_path_str.length(); i++) {
                char16_t ccc = entry_path_str[i];
                fout.write(reinterpret_cast<const char*>(&ccc), sizeof(char16_t));
            }
            fout.write(reinterpret_cast<const char*>(&rentry_fsize), sizeof(rentry_fsize));

            std::uint64_t left = rentry_fsize;
            static constexpr std::uint64_t CHUNK_SIZE = 0x10000;

            std::ifstream fin(entry.path().string(), std::ios_base::binary);
            std::vector<char> buffer;

            while (left > 0) {
                const std::uint64_t take = std::min<std::uint64_t>(CHUNK_SIZE, left);
                buffer.resize(take);

                fin.read(buffer.data(), buffer.size());
                fout.write(buffer.data(), buffer.size());

                left -= take;
            }

            header.file_count++;
        }
    }

    fout.seekp(0, std::ios_base::beg);
    fout.write(reinterpret_cast<const char*>(&header), header.header_size);

    return 0;
}