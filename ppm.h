#include <stdio.h> // for debugging
#include <stdlib.h>

short*loadimg(int*height,int*width,short*canvas){
	int h=0,w=0;char a;
	FILE*f=fopen("save.ppm","r");
	for(int i=0;i<3;i++)getc(f); // P6\n
	while((a=getc(f))!=' ')h=h*10+a-48; // <X> 
	while((a=getc(f))!=10)w=w*10+a-48; // <Y>\n
	canvas=(short*)malloc(h*w*sizeof(short));
	if(!canvas){printf("Failed to allocate canvas\n");exit(1);}
	printf("load %i %i %p\n",w,h,canvas);
	while((a=getc(f))!=10); // 255\n

	for(int x=0;x<w;x++)
	for(int y=0;y<h;y++)
		canvas[y*w+x]=
			(getc(f)>>4)<<8|
			(getc(f)>>4)<<4|
			(getc(f)>>4)<<0; // fancy!
	fclose(f);

	*height=h;
	*width=w;
	return(canvas);
}

void saveimg(int height,int width,short*canvas){
	printf("save %i %i\n",width,height);
	FILE*f=fopen("save.ppm","w");
	fprintf(f,"P6\n%i %i\n255\n",height,width);
	for(int x=0;x<width;x++)
	for(int y=0;y<height;y++){
		putc((canvas[y*width+x]&0xf00)>>4,f);
		putc((canvas[y*width+x]&0x0f0)   ,f);
		putc((canvas[y*width+x]&0x00f)<<4,f);
	}
	fclose(f);
}
