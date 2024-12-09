#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

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
    uint32_t VolumeId;          
    uint8_t VolumeLabel[11];    
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
public:  // Changed to public for direct access in main()
    BootSector m_BootSector;
    uint8_t* m_Fat;
    DirectoryEntry* m_RootDirectory;
    uint32_t m_RootDirectoryEnd;
    FILE* m_DiskImage;

private:
    int readBootSector() {
        return fread(&m_BootSector, sizeof(m_BootSector), 1, m_DiskImage) > 0;
    }

    int readSectors(uint32_t lba, uint32_t count, void* bufferOut) {
        if (fseek(m_DiskImage, lba * m_BootSector.BytesPerSector, SEEK_SET) != 0)
            return 0;
        return fread(bufferOut, m_BootSector.BytesPerSector, count, m_DiskImage) == count;
    }

    int readFat() {
        m_Fat = (uint8_t*)malloc(m_BootSector.SectorsPerFat * m_BootSector.BytesPerSector);
        return readSectors(m_BootSector.ReservedSectors, m_BootSector.SectorsPerFat, m_Fat);
    }

    int readRootDirectory() {
        uint32_t lba = m_BootSector.ReservedSectors + m_BootSector.SectorsPerFat * m_BootSector.FatCount;
        uint32_t size = sizeof(DirectoryEntry) * m_BootSector.DirEntryCount;
        uint32_t sectors = (size / m_BootSector.BytesPerSector);
        if (size % m_BootSector.BytesPerSector > 0)
            sectors++;

        m_RootDirectoryEnd = lba + sectors;
        m_RootDirectory = (DirectoryEntry*)malloc(sectors * m_BootSector.BytesPerSector);
        return readSectors(lba, sectors, m_RootDirectory);
    }

public:
    FATFileReader() : m_Fat(NULL), m_RootDirectory(NULL), m_DiskImage(NULL) {}

    ~FATFileReader() {
        if (m_Fat) free(m_Fat);
        if (m_RootDirectory) free(m_RootDirectory);
        if (m_DiskImage) fclose(m_DiskImage);
    }

    int open(const char* path) {
        m_DiskImage = fopen(path, "rb");
        return m_DiskImage != NULL;
    }

    DirectoryEntry* findFile(const char* name) {
        for (uint32_t i = 0; i < m_BootSector.DirEntryCount; ++i) {
            if (memcmp(name, m_RootDirectory[i].Name, 11) == 0)
                return &m_RootDirectory[i];
        }
        return NULL;
    }

    int readFile(DirectoryEntry* fileEntry, uint8_t* outputBuffer) {
        int ok = 1;
        uint16_t currentCluster = fileEntry->FirstClusterLow;

        do {
            uint32_t lba = m_RootDirectoryEnd + (currentCluster - 2) * m_BootSector.SectorsPerCluster;
            ok = ok && readSectors(lba, m_BootSector.SectorsPerCluster, outputBuffer);
            outputBuffer += m_BootSector.SectorsPerCluster * m_BootSector.BytesPerSector;

            uint32_t fatIndex = currentCluster * 3 / 2;
            if (currentCluster % 2 == 0)
                currentCluster = (*(uint16_t*)(m_Fat + fatIndex)) & 0x0FFF;
            else
                currentCluster = (*(uint16_t*)(m_Fat + fatIndex)) >> 4;

        } while (ok && currentCluster < 0x0FF8);

        return ok;
    }

    int prepare() {
        if (!readBootSector()) {
            fprintf(stderr, "Could not read boot sector!\n");
            return 0;
        }

        if (!readFat()) {
            fprintf(stderr, "Could not read FAT!\n");
            free(m_Fat);
            return 0;
        }

        if (!readRootDirectory()) {
            fprintf(stderr, "Could not read root directory!\n");
            free(m_Fat);
            free(m_RootDirectory);
            return 0;
        }

        return 1;
    }

    void printFile(DirectoryEntry* fileEntry) {
        uint8_t* buffer = (uint8_t*)malloc(fileEntry->Size + m_BootSector.BytesPerSector);
        
        if (buffer && readFile(fileEntry, buffer)) {
            for (size_t i = 0; i < fileEntry->Size; i++) {
                if (isprint(buffer[i])) 
                    fputc(buffer[i], stdout);
                else 
                    printf("<%02x>", buffer[i]);
            }
            printf("\n");
        } else {
            fprintf(stderr, "Could not read file\n");
        }

        if (buffer) free(buffer);
    }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FATFileReader reader;
    if (!reader.open(argv[1])) {
        fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
        return -1;
    }

    if (!reader.prepare()) {
        return -2; // Could not read boot sector (-2)
    }

    DirectoryEntry* fileEntry = reader.findFile(argv[2]);
    if (!fileEntry) {
        fprintf(stderr, "Could not find file %s!\n", argv[2]);
        free(reader.m_Fat);
        free(reader.m_RootDirectory);
        return -5; // Could not find file (-5)
    }

    uint8_t* buffer = (uint8_t*)malloc(fileEntry->Size + reader.m_BootSector.BytesPerSector);
    if (!reader.readFile(fileEntry, buffer)) {
        fprintf(stderr, "Could not read file %s!\n", argv[2]);
        free(reader.m_Fat);
        free(reader.m_RootDirectory);
        free(buffer);
        return -5; // Could not read file (-5)
    }

    for (size_t i = 0; i < fileEntry->Size; i++) {
        if (isprint(buffer[i])) 
            fputc(buffer[i], stdout);
        else 
            printf("<%02x>", buffer[i]);
    }
    printf("\n");

    free(buffer);
    free(reader.m_Fat);
    free(reader.m_RootDirectory);
    return 0;
}