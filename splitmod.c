/*
 * Splits a ProTracker MOD file into song and sample parts for use with
 * Frank Wille's ptPlayer: http://aminet.net/package/mus/play/ptplayer
 * 
 * - <name>.trk : MOD data (header, sample info, song arrangement, patterns)
 * - <name>.smp : MOD samples (to be loaded into Chip RAM)
 * - <name>.smp.i: ASM include (SPLITMOD_SMP_SIZE declaration for assembler)
 * 
 * Usage: SplitMod <name> [/q]
 *
 * Based on source by Frank Wille.
 *
 * Visual Studio 2017 project and additional smp size code
 * Antiriad <jon@autoitscript.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t FileSize(FILE* file, const char* fileName);
static int SplitMod(char* name, unsigned char* mod, size_t len);

int g_quiet = 0;


int main(int argc, char* argv[])
{
	int returnCode = 1;

	if (argc < 2 || argc > 3)
	{
		fprintf(stderr,"\nSplitMod - Antiriad <jon@autoitscript.com>\n\n");
		fprintf(stderr,"Splits a ProTracker module into song and sample data for use with\n");
		fprintf(stderr,"Frank Wille's ptPlayer: http://aminet.net/package/mus/play/ptplayer\n\n");
		fprintf(stderr, "Usage: %s <module> [/q]\n", argv[0]);
		return 1;
	}

	if (argc ==3 && !_stricmp(argv[2],"/q") )
		g_quiet = 1;

	FILE* f = fopen(argv[1], "rb");
	
	if (f)
	{
		const size_t len = FileSize(f, argv[1]);
		if (len)
		{
			unsigned char* modData = malloc(len);
			if (modData)
			{
				if (fread(modData, 1, len, f) == len)
					returnCode = SplitMod(argv[1], modData, len);
				else
					fprintf(stderr, "\nERROR: Read error ('%s').\n", argv[1]);

				free(modData);
			}
			else
				fprintf(stderr, "\nERROR: Out of memory.\n");
		}
		
		fclose(f);
	}
	else
		fprintf(stderr, "\nERROR: Cannot open '%s'.\n", argv[1]);

	return returnCode;
}


static size_t FileSize(FILE* file, const char* fileName)
{
	const long filePos = ftell(file);
	
	if (filePos >= 0)
	{
		/* Seek to end and then back to start to get file size */
		if (fseek(file, 0,SEEK_END) >= 0)
		{
			const long fileSize = ftell(file);
			
			if (fileSize >= 0)
			{
				if (fseek(file, filePos,SEEK_SET) >= 0)
					return (size_t)fileSize;
			}
		}
	}

	fprintf(stderr, "\nERROR: Cannot read file '%s'\n", fileName);
	return 0;
}


static int SplitMod(char* name, unsigned char* mod, size_t len)
{
	unsigned char* pattern;
	unsigned char maxPattern;
	int returnCode = 1;
	int i;

	/* Allocate buffer for new name with .trk or .smp or .i extension */
	char* nameBuf = malloc(strlen(name) + 5);
	if (!nameBuf)
	{
		fprintf(stderr, "\nERROR: Out of memory.\n");
		return 1;
	}

	/* Determine number of patterns */
	for (pattern = &mod[952], maxPattern = 0, i = 0; i < 128; pattern++, i++)
	{
		if (*pattern > maxPattern)
			maxPattern = *pattern;
	}

	/* Sample data is immediately after pattern data */
	const size_t samplesOffset = ((size_t)(maxPattern + 1) << 10) + 1084;

	/* Get length of samples */
	const size_t samplesSize = len - samplesOffset;

	for (;;)
	{
		/* Write song data */
		sprintf(nameBuf, "%s.trk", name);
		FILE* f = fopen(nameBuf, "wb");
		if (!f)
			break;
		size_t bytesWritten = fwrite(mod, 1, samplesOffset, f);
		fclose(f);
		if (bytesWritten != samplesOffset)
			break;
		if (!g_quiet)
			fprintf(stdout, "Written song data: %s (%u bytes).\n", nameBuf, samplesOffset);
		
		/* Write the sample data */
		sprintf(nameBuf, "%s.smp", name);
		f = fopen(nameBuf, "wb");
		if (!f)
			break;
		bytesWritten = fwrite(mod + samplesOffset, 1, samplesSize, f);
		fclose(f);
		if (bytesWritten != samplesSize)
			break;
		if (!g_quiet)
			fprintf(stdout, "Written sample data: %s (%u bytes).\n", nameBuf, samplesSize);
		
		/* Write the sample size assembler includes */
		sprintf(nameBuf, "%s.i", name);
		f = fopen(nameBuf, "wb");
		if (!f)
			break;
		fprintf(f, "SPLITMOD_SONG_SIZE = %u\n", samplesOffset);
		fprintf(f, "SPLITMOD_SMP_SIZE = %u\n", samplesSize);
		fclose(f);
		if (!g_quiet)
			fprintf(stdout, "Written assembler include for MOD_SMP_SIZE: %s.\n", nameBuf);
				
		returnCode = 0;
		
		break;
	}
	
	/* Any errors? */
	if (returnCode)
	{
		fprintf(stderr, "ERROR: Cannot write '%s'.\n", nameBuf);
		return 1;
	}
	
	return 0;
}
