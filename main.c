#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char chunkID[5];
int chunkSize;
char format[5];

char subChunk1ID[5];
int subChunk1Size;
short audioFormat;
short numChannels;
int sampleRate;
int byteRate;
short blockAlign;
short bitsPerSample;

int channelSize;
char subChunk2ID[5];
int subChunk2Size;

float* data;
int resultSize;
float* resultData;

void print()
{
	chunkID[5] = '\0';
	format[5] = '\0';
	subChunk1ID[5] = '\0';
	subChunk2ID[5] = '\0';

	printf("\n============= HEADER INFO =============\n");
	printf(" chunkID:%s\n", chunkID);
	printf(" chunkSize:%d\n", chunkSize);
	printf(" format:%s\n", format);
	printf(" subChunk1ID:%s\n", subChunk1ID);
	printf(" subChunk1Size:%d\n", subChunk1Size);
	printf(" audioFormat:%d\n", audioFormat);
	printf(" numChannels:%d\n", numChannels);
	printf(" sampleRate:%d\n", sampleRate);
	printf(" byteRate:%d\n", byteRate);
	printf(" blockAlign:%d\n", blockAlign);
	printf(" bitsPerSample:%d\n", bitsPerSample);
	printf(" subChunk2ID:%s\n", subChunk2ID);
	printf(" subChunk2Size:%d\n", subChunk2Size);
}

int loadWave(char* filename)
{
	FILE* in = fopen(filename, "rb");

	if (in != NULL)
	{
		printf("Reading %s...\n",filename);

		fread(chunkID, 1, 4, in);
		fread(&chunkSize, 1, 4, in);
		fread(format, 1, 4, in);

		//sub chunk 1
		fread(subChunk1ID, 1, 4, in);
		fread(&subChunk1Size, 1, 4, in);
		fread(&audioFormat, 1, 2, in);
		fread(&numChannels, 1, 2, in);
		fread(&sampleRate, 1, 4, in);
		fread(&byteRate, 1, 4, in);
		fread(&blockAlign, 1, 2, in);
		fread(&bitsPerSample, 1, 2, in);

		//read extra bytes
		if(subChunk1Size == 18)
		{
			short empty;
			fread(&empty, 1, 2, in);
		}

		//sub chunk 2
		fread(subChunk2ID, 1, 4, in);
		fread(&subChunk2Size, 1, 4, in);

		//read data
		int bytesPerSample = bitsPerSample/8;
		int numSamples = subChunk2Size / bytesPerSample;
		data = (float*) malloc(sizeof(float) * numSamples);

		fread(data, 1, bytesPerSample*numSamples, in);

		int i=0;
		float scaler=0;
		short sample=0;
		while(fread(&sample, 1, bytesPerSample, in) == bytesPerSample)
		{
			scaler = (float) sample/3767;
			data[i++] = scaler;
			//printf("%lf ", scaler);
			//printf("%lf ", data[i-1]);
			sample = 0;
			scaler = 0;
		}

		fclose(in);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file: %s\n", filename);
		return 0;
	}
	return 1;
}

int saveWave(char* filename)
{
	FILE* out = fopen(filename, "wb");

	if (out != NULL)
	{
		printf("\nWriting %s...\n",filename);

		fwrite(chunkID, 1, 4, out);
		fwrite(&chunkSize, 1, 4, out);
		fwrite(format, 1, 4, out);

		//sub chunk 1
		fwrite(subChunk1ID, 1, 4, out);
		fwrite(&subChunk1Size, 1, 4, out);
		fwrite(&audioFormat, 1, 2, out);
		fwrite(&numChannels, 1, 2, out);
		fwrite(&sampleRate, 1, 4, out);
		fwrite(&byteRate, 1, 4, out);
		fwrite(&blockAlign, 1, 2, out);
		fwrite(&bitsPerSample, 1, 2, out);

		//read extra bytes
		if(subChunk1Size == 18)
		{
			short empty = 0;
			fwrite(&empty, 1, 2, out);
		}

		//sub chunk 2
		fwrite(subChunk2ID, 1, 4, out);
		fwrite(&subChunk2Size, 1, 4, out);

		//read data
		int bytesPerSample = bitsPerSample / 8;
		int sampleCount =  subChunk2Size / bytesPerSample;

		//impulse response - echo
		int IRSize = 6;
		float IR[IRSize];
		IR[0] = 1.0;
		IR[1] = 1.0;
		IR[2] = 1.0;
		IR[3] = 1.0;
		IR[4] = 1.0;
		IR[5] = 1.0;

		//write the data
		float* newData = (float*) malloc(sizeof(float) * (sampleCount + IRSize - 1));
		float maxSample = -1;
		float MAX_VAL = 32767.f;	//FIXME: find based on bits per sample
		int i;
		for(i=0; i<sampleCount; ++i)
		{
			//convolve
			int j;
			for(j=0; j<IRSize; ++j)
				newData[i+j] += ((float)data[i] / MAX_VAL) * IR[j];

			//Keep track of max value for scaling
			if(i==0)
				maxSample = newData[0];
			else if(newData[i] > maxSample)
				maxSample = newData[i];
		}

		//scale and re write the data
		for(i=0; i < sampleCount + IRSize - 1; ++i)
		{
			newData[i] = (newData[i] / maxSample) ;
			short sample = (short) (newData[i] * MAX_VAL);
			fwrite(&sample, 1, bytesPerSample, out);
		}

		//clean up
		free(newData);
		fclose(out);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file: %s\n", filename);
		return 0;
	}
	return 1;

}

void convovle(float x[], int N, float h[], int M, float y[], int P){

	int n, m;
	// clear output buffers
	for( n=0; n < P; n++){
		y[n] = 0.0;
	}

	// outer loop: process each input value x[n]

	for( n =0; n < N; n++){

		// inner loop process x[n] with each sample of h[n]
		for( m=0; m<M; m++){

			y[n+m] += x[n] * h[m];
		}
	}
}

int main(int argc, char* argv[])
{
	char* filename = "File.wav";
	char* outFilename = "out.wav";

	loadWave(filename);

	float* fileData = data;
	int fileSize = subChunk2Size;
	resultSize = subChunk2Size;

	print();

	char* IRfileName = "IRTaj.wav";
	loadWave(IRfileName);
	print();

	float* IRData = data;
	int IRSize = subChunk2Size;
	resultSize = resultSize + subChunk2Size;

	resultSize = (resultSize/2) -1;


	resultData =(float*)malloc(sizeof(float) * resultSize);

	//convovle(fileData, fileSize, IRData, IRSize, resultData, resultSize);
	//saveWave(outFilename);
	free(data);

	return 0;
}


