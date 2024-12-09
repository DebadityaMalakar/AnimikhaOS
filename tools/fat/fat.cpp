#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <cstdint>

// Use standard types instead of custom typedefs
struct BootSector {
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];
} __attribute__((packed));

struct DirectoryEntry {
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed));

class FATFileReader {
private:
    BootSector m_BootSector;
    std::vector<uint8_t> m_Fat;
    std::vector<DirectoryEntry> m_RootDirectory;
    uint32_t m_RootDirectoryEnd;
    std::ifstream m_DiskImage;

    bool readBootSector() {
        return m_DiskImage.read(reinterpret_cast<char*>(&m_BootSector), sizeof(m_BootSector)).gcount() == sizeof(m_BootSector);
    }

    bool readSectors(uint32_t lba, uint32_t count, char* bufferOut) {
        m_DiskImage.seekg(lba * m_BootSector.BytesPerSector);
        if (!m_DiskImage) return false;

        m_DiskImage.read(bufferOut, count * m_BootSector.BytesPerSector);
        return m_DiskImage.gcount() == count * m_BootSector.BytesPerSector;
    }

    bool readFat() {
        m_Fat.resize(m_BootSector.SectorsPerFat * m_BootSector.BytesPerSector);
        return readSectors(m_BootSector.ReservedSectors, m_BootSector.SectorsPerFat, reinterpret_cast<char*>(m_Fat.data()));
    }

    bool readRootDirectory() {
        uint32_t lba = m_BootSector.ReservedSectors + m_BootSector.SectorsPerFat * m_BootSector.FatCount;
        uint32_t size = sizeof(DirectoryEntry) * m_BootSector.DirEntryCount;
        uint32_t sectors = (size / m_BootSector.BytesPerSector);
        if (size % m_BootSector.BytesPerSector > 0)
            sectors++;

        m_RootDirectoryEnd = lba + sectors;
        m_RootDirectory.resize(sectors * m_BootSector.BytesPerSector / sizeof(DirectoryEntry));
        return readSectors(lba, sectors, reinterpret_cast<char*>(m_RootDirectory.data()));
    }

public:
    FATFileReader(const std::string& diskImagePath) {
        m_DiskImage.open(diskImagePath, std::ios::binary);
        if (!m_DiskImage) {
            throw std::runtime_error("Cannot open disk image");
        }
    }

    DirectoryEntry* findFile(const char* name) {
        for (auto& entry : m_RootDirectory) {
            if (memcmp(name, entry.Name, 11) == 0) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::vector<uint8_t> readFile(DirectoryEntry* fileEntry) {
        std::vector<uint8_t> buffer(fileEntry->Size + m_BootSector.BytesPerSector);
        uint8_t* currentBuffer = buffer.data();
        uint16_t currentCluster = fileEntry->FirstClusterLow;

        do {
            uint32_t lba = m_RootDirectoryEnd + (currentCluster - 2) * m_BootSector.SectorsPerCluster;
            if (!readSectors(lba, m_BootSector.SectorsPerCluster, reinterpret_cast<char*>(currentBuffer))) {
                throw std::runtime_error("Could not read file clusters");
            }
            currentBuffer += m_BootSector.SectorsPerCluster * m_BootSector.BytesPerSector;

            uint32_t fatIndex = currentCluster * 3 / 2;
            if (currentCluster % 2 == 0)
                currentCluster = (*(uint16_t*)(m_Fat.data() + fatIndex)) & 0x0FFF;
            else
                currentCluster = (*(uint16_t*)(m_Fat.data() + fatIndex)) >> 4;

        } while (currentCluster < 0x0FF8);

        return buffer;
    }

    void processAndPrintFile(const std::string& fileName) {
        DirectoryEntry* fileEntry = findFile(fileName.c_str());
        if (!fileEntry) {
            throw std::runtime_error("Could not find file: " + fileName);
        }

        auto buffer = readFile(fileEntry);

        for (size_t i = 0; i < fileEntry->Size; i++) {
            if (std::isprint(buffer[i])) {
                std::cout << static_cast<char>(buffer[i]);
            } else {
                std::printf("<%02x>", buffer[i]);
            }
        }
        std::cout << std::endl;
    }

    void readAndPrepare() {
        if (!readBootSector()) {
            throw std::runtime_error("Could not read boot sector");
        }

        if (!readFat()) {
            throw std::runtime_error("Could not read FAT");
        }

        if (!readRootDirectory()) {
            throw std::runtime_error("Could not read root directory");
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Syntax: " << argv[0] << " <disk image> <file name>" << std::endl;
        return -1;
    }

    try {
        FATFileReader reader(argv[1]);
        reader.readAndPrepare();
        reader.processAndPrintFile(argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}