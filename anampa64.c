/*
  List Data Analyzer for MPA-3
  written by t.ohsaki 2003/5/2
  modified for 64-bit by s.yanagida 2014/11/22
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


#define	PH_Discri	10	/* Pulse Height Discri. Channel */
#define	TOF_Discri	10	/* TOF Discri. Channel */

/* buffer size of List Data */

#define Buffer_H 8192
#define Buffer_NL 131072

#define Char (sizeof(unsigned char))
#define Short (sizeof(unsigned short))
#define Word (sizeof(unsigned int))

#define Tmp_Size 10

#define Timer_Tag 0x4000
#define Data_Tag 0xffff
#define ADC1 (0x0001)
#define ADC2 (0x0001 << 1)
#define Both_Adc 0x0000
#define Either_Adc 0x8000

unsigned short upper(unsigned int x);
unsigned short lower(unsigned int x);


int main(int argc,char *argv[]) {
    unsigned int i, j;
    unsigned char *MpaHeader;
    unsigned int *MpaList;
    unsigned int mpa_length;
    FILE *fpi, *fpo;
    struct stat buf;
    char tmp_char[Tmp_Size];
    unsigned int adc1_mem, adc2_mem;
    double live_time;
    unsigned int read_count;
    unsigned int real_count,adc1_live_count, adc2_live_count;
    unsigned int header, dead_or_alive;
    unsigned short *NewList, adc1, adc2;
    unsigned int *adc1_hist, *adc2_hist;
    unsigned int adc1_mask, adc2_mask;
    unsigned int adc_coin, adc1_only, adc2_only;
    unsigned int adc1_out_range, adc2_out_range;
    char name[50], ext1[5]="tof", ext2[5]="ph";


    if(argc != 3){
        printf("\aUsage\n anampa mpa_list new_list\n");
        exit(1);
    }

    if((fpi=fopen((char *)argv[1],"rb"))== NULL){
        printf("Can't open%s\n",argv[1]);
        exit(1);
    }

    stat(argv[1],&buf);
    mpa_length=(unsigned int)buf.st_size;

    if ((fpo=fopen((char *)argv[2],"wb")) == NULL){
        printf("Can't create %s",argv[2]);
        exit(1);
    }

    if( (MpaHeader=(unsigned char *)calloc(Buffer_H,Char)) == NULL ){
        puts("Not Enough Memory for MpaHeader");
        exit(1);
    }

    fread(MpaHeader,Buffer_H,Char,fpi); // read header file
    i=0;

    while( !( (MpaHeader[i] == 'A') && (MpaHeader[i+1] == 'D') &&
              (MpaHeader[i+2] == 'C') && (MpaHeader[i+3] == '1') ) ){
        i++;
        if(i > Buffer_H){
            puts("Error in finding [ADC1]");
            exit(1);
        }
    }
    i+=13; // [ADC1]CrLf range='x'
    tmp_char[0]=MpaHeader[i]; /* scan adc1 memory size */
    for(j=1;j<Tmp_Size;j++){
        if( MpaHeader[i++] == 0x0d ){ // 0x0d = Cr
            break;
        }
        tmp_char[j]=MpaHeader[i];
    }
    tmp_char[j]='\0';
    adc1_mem=atoi(tmp_char);

    while( !( (MpaHeader[i] == 'l') && (MpaHeader[i+1] == 't') &&
              (MpaHeader[i+2] == 'p') && (MpaHeader[i+3] == 'r') ) ){
        i++;
        if(i > Buffer_H){
            puts("Error in finding ltpreset");
            exit(1);
        }
    }
    i+=9; // ltpreset='x'
    tmp_char[0]=MpaHeader[i]; // scan measurement live time
    for(j=1;j<Tmp_Size;j++){
        if( MpaHeader[i++] == 0x0d || MpaHeader[i] == 0x7f ){ // 0x0d = Cr
            break; // 0x7f='space'
        }
        tmp_char[j]=MpaHeader[i];
    }
    tmp_char[j]='\0';
    live_time=atof(tmp_char);

    while( !( (MpaHeader[i] == 'A') && (MpaHeader[i+1] == 'D') &&
              (MpaHeader[i+2] == 'C') && (MpaHeader[i+3] == '2') ) ){
        i++;
        if(i > Buffer_H){
            puts("Error in finding [ADC2]");
            exit(1);
        }
    }
    i+=13; // [ADC2]CrLf range='x'
    tmp_char[0]=MpaHeader[i]; // adc2 memory size
    for(j=1;j<Tmp_Size;j++){
        if( MpaHeader[i++] == 0x0d ){ // 0x0d = Cr
            break;
        }
        tmp_char[j]=MpaHeader[i];
    }
    tmp_char[j]='\0';
    adc2_mem=atoi(tmp_char);

    while( !( (MpaHeader[i] == 'L') && (MpaHeader[i+1] == 'I') &&
              (MpaHeader[i+2] == 'S') && (MpaHeader[i+3] == 'T') ) ){
        i++;
        if(i > Buffer_H){
            puts("Error in finding [LISTDATA]");
            exit(1);
        }
    }

    i+=10; // [LISTDATA]CrLf
    mpa_length-=(i+1)*Char;
    fseek(fpi,(long)(i+1),SEEK_SET);

    free(MpaHeader);

    read_count=mpa_length/Word;
    if( (MpaList=(unsigned int *)calloc(read_count,Word)) == NULL ){
        puts("Not Enough Memory for MpaList");
        exit(1);
    }
    fread(MpaList,read_count,Word,fpi); // read mpa list data
    fclose(fpi);

    if( (NewList=(unsigned short *)calloc(Buffer_NL*2,Short)) == NULL ){
        puts("Not Enought Memory for NewList");
        exit(1);
    }
    if( (adc1_hist=(unsigned int *)calloc(adc1_mem,Word)) == NULL ){
        puts("Not Enought Memory for adc1_hist");
        exit(1);
    }
    if( (adc2_hist=(unsigned int *)calloc(adc2_mem,Word)) == NULL ){
        puts("Not Enought Memory for adc2_hist");
        exit(1);
    }

    real_count=0;
    adc1_live_count=0;
    adc2_live_count=0;
    adc_coin=0;
    adc1_only=0;
    adc2_only=0;
    adc1_out_range=0;
    adc2_out_range=0;

    adc1_mask=0;
    for(i=0;i<=(unsigned int)(log((double)adc1_mem)/log(2.0));i++){
        adc1_mask+=(unsigned int)pow(2.0,(double)i);
    }
    adc2_mask=0;
    for(i=0;i<=(unsigned int)(log((double)adc2_mem)/log(2.0));i++){
        adc2_mask+=(unsigned int)pow(2.0,(double)i);
    }

    i=0;
    j=0;
    do{
        header=upper(MpaList[i]);
        if(header == Timer_Tag){
            real_count++;
            dead_or_alive=lower(MpaList[i]);
            if( (dead_or_alive & ADC1) == ADC1 ){
                adc1_live_count++;
            }
            if( (dead_or_alive & ADC2) == ADC2 ){
                adc2_live_count++;
            }
        }
        else if(header == Data_Tag){
            // puts("Data found1");
            if( lower(MpaList[i]) != Data_Tag ){
                puts("Data Error0 at List Data");
                exit(1);
            }
            if(upper(MpaList[i+1]) == Timer_Tag){
                puts("Data Error1 at List Data");
                exit(1);
            }
            // short a = 0xffff;
            // fwrite(, Short, Buffer_NL, fpo); // added by yanagida
        }
        else if(header == Both_Adc){
            // puts("Data found2");
            if(lower(MpaList[i]) == (ADC1 | ADC2)){
                adc1=lower(MpaList[++i]);
                adc2=upper(MpaList[i]);
                if( (adc1 < 0) || (adc1_mem <= adc1) ){
                    adc1_out_range++;
                }
                if( (adc2 < 0) || (adc2_mem <= adc2) ){
                    adc2_out_range++;
                }
                adc1 &= adc1_mask;
                adc2 &= adc2_mask;
                adc1_hist[adc1]++;
                adc2_hist[adc2]++;
                if( (adc1 >= TOF_Discri) && (adc2 >= PH_Discri) ){
                    NewList[j++]=adc1;
                    NewList[j++]=adc2;
                }
                if( j == Buffer_NL*2 ){
                    fwrite(NewList,Short,Buffer_NL*2,fpo);
                    j=0;
                }
                adc_coin++;
                // printf("[adc1] = %d\n",lower(MpaList[i]));
                // printf("[adc2] = %d\n",upper(MpaList[i]));
            }
            else{
                puts("Data Error2 at List Data");
                for(j=0;j<10;j++){
                    printf("Data = %08x\n",MpaList[i+j]);
                }
                exit(1);
            }
        }
        else if(header == Either_Adc){
            // puts("Data found3");
            if( (lower(MpaList[i]) & ADC1) == ADC1 ){
                adc1=upper(MpaList[++i]);
                if( (adc1 < 0) || (adc1_mem <= adc1) ){
                    adc1_out_range++;
                }
                adc1 &= adc1_mask;
                adc1_hist[adc1]++;
                adc1_only++;
                // printf("[adc1] = %d\n",upper(MpaList[i]));
            }
            else if( (lower(MpaList[i]) & ADC2) == ADC2 ){
                adc2=upper(MpaList[++i]);
                if( (adc2 < 0) || (adc2_mem <= adc2) ){
                    adc2_out_range++;
                }
                adc2 &= adc2_mask;
                adc2_hist[adc2]++;
                adc2_only++;
                // printf("[adc2] = %d\n",upper(MpaList[i]));
            }
            else{
                puts("Data Error3 at List Data");
                exit(1);
            }
        }
        else{
            puts("Data Error4 at List Data");
            printf("data = %08x\n",MpaList[i]);
            exit(1);
        }
    }while( ++i < read_count );
    fwrite(NewList,Short,j,fpo);
    fclose(fpo);

    free(MpaList);
    free(NewList);

    strcpy(name,argv[1]);
    name[strlen(argv[1])-3]='\0';
    strcat(name,ext1);
    if ((fpo=fopen(name,"wt")) == NULL){
        printf("Can't create %s",name);
        exit(1);
    }
    for(i=0;i<adc1_mem;i++){
        fprintf(fpo,"%u\n",adc1_hist[i]);
    }
    fclose(fpo);

    strcpy(name,argv[1]);
    name[strlen(argv[1])-3]='\0';
    strcat(name,ext2);
    if ((fpo=fopen(name,"wt")) == NULL){
        printf("Can't create %s",name);
        exit(1);
    }
    for(i=0;i<adc2_mem;i++){
        fprintf(fpo,"%u\n",adc2_hist[i]);
    }
    fclose(fpo);

    free(adc1_hist);
    free(adc2_hist);

    printf("\n");
    printf("ADC1 Memory Size = %u\n",adc1_mem);
    printf("ADC2 Memory Size = %u\n",adc2_mem);
    printf("Live Time = %.1f [sec] \n",live_time);
    printf("\n");
    printf("Real_Count = %u\n",real_count);
    printf("ADC1_Live_Count = %u\n",adc1_live_count);
    printf("ADC2_Live_Count = %u\n",adc2_live_count);
    printf("ADC_Coin = %u\n",adc_coin);
    printf("ADC1_Only = %u\n",adc1_only);
    printf("ADC2_Only = %u\n",adc2_only);
    printf("ADC1 Out Range Data = %u\n",adc1_out_range);
    printf("ADC2 Out Range Data = %u\n",adc2_out_range);
    return 0;
}

unsigned short upper(unsigned int x)
{
    x = x >> 16;
    return((unsigned short)(x & 0x0000ffff));
}

unsigned short lower(unsigned int x)
{
    return((unsigned short)(x & 0x0000ffff));
}
