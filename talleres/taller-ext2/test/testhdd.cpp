#include <iostream>
#include <assert.h>
#include <stdlib.h>

#include "hdd.h"
#include "mbr.h"
#include "pentry.h"
#include "ext2fs.h"


using namespace std;

void test_file_system(Ext2FS * fs){
	cout << "=== Filesystem Superblock ===" << endl;
	cout << *(fs->superblock()) << endl;

	//Verifico que la informacion de la cantidad de bloques sea la esperada
	assert(fs->superblock()->blocks_count == 102400);
}

void test_hdd(HDD * hdd){
	unsigned char buffer[SECTOR_SIZE];
	hdd->read(0, buffer);
	MBR mbr(buffer);
	const PartitionEntry & pentry = mbr[1];
	cout << "=== Partition Data ===" << endl;
	cout << pentry << endl << endl;

	//Verifico que la LBA empiece donde se espera
	assert(pentry.start_lba() == 4096);
}

void test_block_groups(Ext2FS * fs){
	cout << "=== Block Groups Data ===" << endl;
	unsigned int block_groups = fs->block_groups();
	for(unsigned int i = 0; i < block_groups; i++)
	{
		cout << *(fs->block_group(i)) << endl;
	}
	Ext2FSBlockGroupDescriptor block_group = *(fs->block_group(1));

	//Verifico que el block group 1 tenga la información correcta
	assert(block_group.block_bitmap == 8195);
}

int main(int argc, char ** argv)
{
	if (argc != 3) {
		printf("Uso: %s hdd.raw /grupos/g4/nota.txt\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	HDD hdd(argv[1]);
	Ext2FS * fs = new Ext2FS(hdd, 1);

	// test_hdd(&hdd);
	// test_file_system(fs);
	// test_block_groups(fs);

	fd_t file_descriptor = fs->open(argv[2], "r");

	// avanzo el file descriptor la cantidad de bytes que indica el enunciado
	fs->seek(file_descriptor, 13999);

	// creo un buffer para leer el archivo, seteando la ultima posicion en 0
	// (en C, los strings son null-terminated)
	unsigned char buffer[18];
	buffer[17] = 0;

	// leo el archivo en el buffer
	fs->read(file_descriptor, (unsigned char*) buffer, 17);

	// imprimo lo que lei del archivo
	printf("Contenido leido del archivo: %s\n", buffer);

	// cierro el file descriptor
	fs->close(file_descriptor);

	return 0;
}
