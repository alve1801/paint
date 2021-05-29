#define OLC_PGE_APPLICATION
#include <stdlib.h>
#include <stdio.h> // mostly for debugging
#include "olc.h"
#include "ppm.h"
#define tilesize 32
#define tilenum 7
#define sh 400
#define sw 300
#define ss 3 // screen scale
#define offset 0

short palette[16]={ // legacy - we dont use this
	0x000,0x00a,0x0a0,0x0aa, 0xa00,0xa0a,0xa50,0xaaa,
 	0x555,0x55f,0x5f5,0x5ff, 0xf55,0xf5f,0xff5,0xfff
};

// this is actually only 90 unique colors, but I havent found a reliable way to speed it up
short hsvpalette[256]={0xf00,0xf00,0xf00,0xf10,0xf10,0xf10,0xf20,0xf20,0xf30,0xf30,0xf30,0xf40,0xf40,0xf40,0xf50,0xf50,0xf60,0xf60,0xf60,0xf70,0xf70,0xf70,0xf80,0xf80,0xf80,0xf90,0xf90,0xf90,0xfa0,0xfa0,0xfb0,0xfb0,0xfb0,0xfc0,0xfc0,0xfd0,0xfd0,0xfd0,0xfe0,0xfe0,0xfe0,0xff0,0xff0,0xff0,0xff0,0xef0,0xef0,0xef0,0xdf0,0xdf0,0xdf0,0xcf0,0xcf0,0xcf0,0xbf0,0xbf0,0xaf0,0xaf0,0xaf0,0x9f0,0x9f0,0x8f0,0x8f0,0x8f0,0x7f0,0x7f0,0x7f0,0x6f0,0x6f0,0x5f0,0x5f0,0x5f0,0x5f0,0x4f0,0x4f0,0x3f0,0x3f0,0x3f0,0x2f0,0x2f0,0x1f0,0x1f0,0x1f0,0x0f0,0x0f0,0x0f0,0x0f0,0x0f0,0x0f1,0x0f1,0x0f1,0x0f2,0x0f2,0x0f3,0x0f3,0x0f3,0x0f4,0x0f4,0x0f4,0x0f5,0x0f5,0x0f5,0x0f6,0x0f6,0x0f6,0x0f7,0x0f7,0x0f8,0x0f8,0x0f8,0x0f9,0x0f9,0x0fa,0x0fa,0x0fa,0x0fb,0x0fb,0x0fb,0x0fc,0x0fc,0x0fc,0x0fd,0x0fd,0x0fe,0x0fe,0x0fe,0x0ff,0x0ff,0x0ff,0x0ff,0x0ff,0x0ef,0x0ef,0x0df,0x0df,0x0df,0x0cf,0x0cf,0x0cf,0x0bf,0x0bf,0x0af,0x0af,0x0af,0x09f,0x09f,0x09f,0x08f,0x08f,0x08f,0x07f,0x07f,0x07f,0x06f,0x06f,0x05f,0x05f,0x05f,0x04f,0x04f,0x04f,0x03f,0x03f,0x02f,0x02f,0x02f,0x01f,0x01f,0x00f,0x00f,0x00f,0x00f,0x00f,0x00f,0x10f,0x10f,0x20f,0x20f,0x20f,0x30f,0x30f,0x30f,0x40f,0x40f,0x50f,0x50f,0x50f,0x60f,0x60f,0x60f,0x70f,0x70f,0x80f,0x80f,0x80f,0x90f,0x90f,0x90f,0xa0f,0xa0f,0xa0f,0xb0f,0xb0f,0xc0f,0xc0f,0xc0f,0xd0f,0xd0f,0xd0f,0xe0f,0xe0f,0xf0f,0xf0f,0xf0f,0xf0f,0xf0f,0xf0e,0xf0e,0xf0e,0xf0d,0xf0d,0xf0c,0xf0c,0xf0c,0xf0b,0xf0b,0xf0b,0xf0a,0xf0a,0xf09,0xf09,0xf09,0xf08,0xf08,0xf08,0xf07,0xf07,0xf07,0xf06,0xf06,0xf05,0xf05,0xf05,0xf04,0xf04,0xf04,0xf03,0xf03,0xf02,0xf02,0xf02,0xf01,0xf01,0xf01,0xf00,0xf00};

int height=64,width=64; // canvas size
short rin=0x0000,tmp,*canvas,*clipboard,tiles[128*256],newmap[sh][sw],oldmap[sh][sw];
int cox=0,coy=0,psc=1,bx,by;
unsigned char op=0; // pencil spray line0 line picker bucket rect0 rect frect0 frect select place
bool gridon=false;
int brushsize=8,density=10;

// god damn it c
int min(int a,int b){return(a<b?a:b);}
int max(int a,int b){return(a>b?a:b);}

void clear(){for(int i=0;i<height*width;i++)canvas[i]=0x0fff;} // aka memset, but i was too lazy to import unistd.h for just one god damn function that isnt even that hard to write myself

void p(int x,int y){if(x>=0 && x<height && y>=0 && y<width)canvas[x*width+y]=rin;}

void bucket(int x,int y,short s){
	if(canvas[x*width+y]==rin)return;
	if(canvas[x*width+y]!=s)return;
	canvas[x*width+y]=rin;
	if(x>0)bucket(x-1,y,s);
	if(y>0)bucket(x,y-1,s);
	if(x<height-1)bucket(x+1,y,s);
	if(y<width-1)bucket(x,y+1,s);
}

void drawline(int x1,int y1,int x2,int y2){ // dont touch this
	int dx=x1-x2,dy=y1-y2;
	if(abs(dx)<abs(dy)){
		if(y1<y2) for(int i=y1;i<y2;i++) p((i-y1)*dx/dy+x1, i);
		else      for(int i=y2;i<y1;i++) p((i-y2)*dx/dy+x2, i);
	}else{
		if(x1<x2) for(int i=x1;i<x2;i++) p(i, (i-x1)*dy/dx+y1);
		else      for(int i=x2;i<x1;i++) p(i, (i-x2)*dy/dx+y2);
	}
	p(x1,y1);p(x2,y2);
}

void spray(int bx,int by){ // XXX remove dither
	// also maybe use gaussian distribution?
	char bayer[2][2]={0,2,3,1}; // bayer matrix used for dithering
	//bayer[4][4]={4,12,6,14,8,0,10,2,7,15,5,13,11,3,9,1};
	for(int x=-brushsize;x<brushsize;x++)
	for(int y=-brushsize;y<brushsize;y++)
	//if(x*x+y*y<brushsize*brushsize && rand()%100<density && rand()%4<=bayer[(x+bx)%2][(y+by)%2]) // XXX still rather inprecise
	if(x*x+y*y<brushsize*brushsize && rand()%100<density)
	p(x+bx,y+by);
}

void flip(){
	short t;
	for(int y=0;y<width;y++)
	for(int x=0;x<height/2;x++)
	{
		t=canvas[x*width+y];
		canvas[x*width+y]=canvas[(height-x-1)*width+y];
		canvas[(height-x-1)*width+y]=t;
	}
}

short hsv(short rgb){
	// XXX seems to have issues when two colors have the same value
	char r = (rgb&0xf00)>>8;
	char g = (rgb&0xf0)>>4;
	char b =  rgb&0xf;
	char maxcolor=max(r,max(g,b)),mincolor=min(r,min(g,b));
	if(maxcolor==mincolor)return r;

	char h,v=(maxcolor+mincolor)/2;
	char s=(maxcolor-mincolor)*0xf/(v<9?maxcolor+mincolor:0x20-maxcolor-mincolor);
	if(maxcolor==r)h=(g-b)/(maxcolor-mincolor);
	else if(maxcolor==g)h=0x20+(b-r)/(maxcolor-mincolor);
	else h=0x40+(r-g)/(maxcolor-mincolor);
	h/=6;
	h%=0xf;

	printf("hsv %i %i %i\n",h,s,v);
	return (h<<8)|(s<<4)|v;
}

short rgb(short hsv){
	char h = (hsv&0xf00)>>8;
	char s = (hsv&0xf0)>>4;
	char v =  hsv&0xf;

	if(s==0){return (v<<8)|(v<<4)|v;}

	char r,g,b,t1,t2;

	/*
	// t1 is low, t2 is high - i think
	if(v<9)t2=v*(0xf+s); // v+v*s
	else t2=(v+s)-(v*s); // wat
	t1=0x20*v-t2;

	r=h+5;r%=0xf;
	g=h;
	b=h-5;b%=0xf;

	//Red
	if(r<4) r = t1+(t2-t1)*6*r; // 1/6
	else if(r<9)r=t2;
	else if(r<12)r=t1+(t2-t1)*(12-r)*6; // 2/3
	else r=t1;

	//Green
	if(g<4)g=t1+(t2-t1)*6*g; // 1/6
	else if(g<9)g=t2;
	else if(g<12)g=t1+(t2-t1)*(12-g)*6; // 2/3
	else g=t1;

	//Blue
	if(b<4)b=t1+(t2-t1)*6*b; // 1/6
	else if(b<9)b=t2;
	else if(b<12)b=t1+(t2-t1)*(12-b)*6; // 2/3
	else b = t1;

	printf("rgb %i %i %i\n",r,g,b);

	return (r<<8)|(g<<4)|b;
	*/

	// ok idea: we do it ourselves
	// use a lookup table for h
	// rgb = rgb*s+.5*(1-s)
	// rgb*=v

	r=hsvpalette[h]>>8;
	g=(hsvpalette[h]&0xf0)>>4;
	b=hsvpalette[h]&0xf;

	t1=8*(0xf-s)*v;
	r=r*s*v+t1;
	g=g*s*v+t1;
	b=b*s*v+t1;
	r/=16;
	g/=16;
	b/=16;

	printf("rgb %i %i %i\n",r,g,b);

	return (r<<8)|(g<<4)|b;
}



class View:public olc::PixelGameEngine{
	public:View(){sAppName="MS Paint but cooler";}
	bool OnUserCreate()override{return(render(0,0));}

	olc::Pixel sga(short color){ // transforms color from 8bit sga space to 24bit truecolor (strictly speaking its 12bit but whatever)
		int r = (color&0x0f00)>>4;
		int g = (color&0x00f0);
		int b = (color&0x000f)<<4;

		r=r|r>>4;
		g=g|g>>4;
		b=b|b>>4;

		return olc::Pixel(r,g,b);
	}

	bool render(int mx,int my){
		// clear newmap
		for(int x=0;x<sh;x++)for(int y=0;y<sw;y++)
		newmap[x][y]=(x^y)&1?0x0444:0x0000;


		// draw canvas
		for(int x=0;x<height;x++)
		for(int y=0;y<width;y++)
		for(int sx=0;sx<psc;sx++)
		for(int sy=0;sy<psc;sy++){
			if(
				x*psc+sx+cox +tilesize*2 >0 &&
				x*psc+sx+cox +tilesize*2 <sh &&
				y*psc+sy+coy >0 &&
				y*psc+sy+coy <sw
			){
				if(gridon && (psc>1) && (sx==0 || sy==0) && (x%8==0 || y%8==0)) // XXX last few get weird
					newmap[x*psc+sx+cox +tilesize*2][y*psc+sy+coy]=0x0000;
				else
					newmap[x*psc+sx+cox +tilesize*2][y*psc+sy+coy]=canvas[x*width+y];
			}
		}

		// note to self about the following: the tile file is 4 tiles wide, even though we never use the second half
		// thanks bro

		// draw tile buttons
		for(int x=0;x<tilenum;x++)for(int y=0;y<2;y++)
		for(int sx=0;sx<tilesize;sx++)for(int sy=0;sy<tilesize;sy++)
		newmap[y*tilesize+sy][x*tilesize+sx]=tiles[sx*tilesize*4+sy +tilesize*2];

		// draw selected button XXX
		//for(int sx=0;sx<tilesize;sx++)for(int sy=0;sy<tilesize;sy++)
		//newmap[[0,1,0,0,1,0,0,0,1,1][op]*tilesize+sy][[2,2,3,3,3,4,5,5,5,5][op]*tilesize+sx]=tiles[sx*tilesize*4+sy +tilesize*3]; // aaargh!
		// we can totes use bitshifting for that


		// draw tiles
		for(int x=0;x<tilesize*tilenum;x++)
		for(int y=0;y<tilesize*2;y++)
		if(tiles[x*tilesize*4+y]!=0xfff)newmap[y][x]=tiles[x*tilesize*4+y];

		// draw color picks
		int ay=tilenum*tilesize,cy=sw,x,y;unsigned char ty;
		for(y=ay;y<cy;y++){
			x=0;ty=(char)(16-((float)y-ay+1)/(cy-ay)*16);
			for(;x<tilesize/2  ;x++)newmap[x][y]=rin;
			for(;x<tilesize    ;x++)newmap[x][y]=ty<<8 | rin&0x0ff;
			for(;x<tilesize*3/2;x++)newmap[x][y]=ty<<4 | rin&0xf0f;
			for(;x<tilesize*2-1;x++)newmap[x][y]=ty<<0 | rin&0xff0;
		}


		// draw color selector bars
		x=tilesize/2,y=(cy-ay)/16+1;
		for(;x<tilesize    ;x++){newmap[x][sw-((rin&0xf00)>>8)*y]=0xfff;newmap[x][sw-(((rin&0xf00)>>8)+1)*y]=0xfff;}
		for(;x<tilesize*3/2;x++){newmap[x][sw-((rin&0x0f0)>>4)*y]=0xfff;newmap[x][sw-(((rin&0x0f0)>>4)+1)*y]=0xfff;}
		for(;x<tilesize*2-1;x++){newmap[x][sw-((rin&0x00f)>>0)*y]=0xfff;newmap[x][sw-(((rin&0x00f)>>0)+1)*y]=0xfff;}

		// @ milad - ich empfehle dir, dein code hier einzufugen ;D

		// draw brush
		if(mx>tilesize*2){
			mx=int((mx-cox)/psc)*psc+cox;
			my=int((my-coy)/psc)*psc+coy;
			for(int sx=0;sx<psc;sx++)
			for(int sy=0;sy<psc;sy++)
			//if((mx+sx)%3 == (my+sy)%3)
			if(sx==0||sy==0||sx==psc-1||sy==psc-1)
			switch(op){
			case 0:newmap[mx+sx][my+sy]=0xf00+(sx^sy)<<3;break;
			case 1:
				newmap[mx+sx][my+sy]=0x00f;
				// XXX circle
				for(int x=-brushsize;x<brushsize;x+=2)
				for(int y=-brushsize;y<brushsize;y+=2)
				if(x*x+y*y<brushsize*brushsize)
				newmap[mx+sx+x*psc][my+sy+y*psc]=0x00f;
				break;
			case 2:newmap[mx+sx][my+sy]=0xff0;break;
			case 3:{
				newmap[mx+sx][my+sy]=0xff0;
				int x1=mx+sx,y1=my+sy,x2=bx*psc+tilesize*2+sx,y2=by*psc+sy;int dx=x1-x2,dy=y1-y2;
				//printf("from:<%i %i> to:<%i %i> diff:<%i %i>\n",x1,y1,x2,y2,dx,dy);
				if(abs(dx)!=0 || abs(dy)!=0)
				if(abs(dx)<abs(dy)){
					if(y1<y2) for(int i=y1;i<=y2;i+=psc) newmap[(i-y1)*dx/dy+x1][i]=0xff0;
					else      for(int i=y2;i<=y1;i+=psc) newmap[(i-y2)*dx/dy+x2][i]=0xff0;
				}else{
					if(x1<x2) for(int i=x1;i<=x2;i+=psc) newmap[i][(i-x1)*dy/dx+y1]=0xff0;
					else      for(int i=x2;i<=x1;i+=psc) newmap[i][(i-x2)*dy/dx+y2]=0xff0;
				}
				}
				break;
			case 5:newmap[mx+sx][my+sy]=0x0ff;break;
			case 6:case 7:newmap[mx+sx][my+sy]=0xf0f;break; // XXX draw rectangle
			case 8:case 9:newmap[mx+sx][my+sy]=0x808;break; // XXX draw rectangle
			default:newmap[mx+sx][my+sy]=0xbd5;break;
			}
		}

		// update screen
		for(int x=0;x<sh;x++)for(int y=0;y<sw;y++)
		if(newmap[x][y]!=oldmap[x][y]){
			oldmap[x][y]=newmap[x][y];
			Draw(x,y,sga(newmap[x][y]));
		}

		return true;
	}

	bool OnUserUpdate(float fElapsedTime)override{
		int x=GetMouseX(),y=GetMouseY();
		int lox=(x-tilesize*2-cox)/psc,loy=(y-coy)/psc; // in local space

		//printf(">%i(%i) %i(%i) : %i\n",x,lox,y,loy,op);

		if(GetMouse(0).bPressed){
			if(x>tilesize*2){switch(op){ // paint mode - select according to wtf we currently are doing
				case 0:bx=lox;by=loy;break;
				case 1:break; // handled in bHeld
				case 2:bx=lox;by=loy;op=3;break;
				case 3:drawline(lox,loy,bx,by);bx=lox;by=loy;break; // noice
				case 4:rin=canvas[lox*width+loy];op=0;break;
				case 5:saveimg(height,width,canvas);bucket(lox,loy,canvas[lox*width+loy]);break;
				case 6:bx=lox;by=loy;op=7;break;
				case 7:
					drawline(bx,loy,bx,by);
					drawline(lox,by,bx,by);
					drawline(lox,loy,lox,by);
					drawline(lox,loy,bx,loy);
					op=6;break;
				case 8:bx=lox;by=loy;op=9;break;
				case 9:
					for(int x=min(bx,lox);x<max(bx,lox)+1;x++)
					for(int y=min(by,loy);y<max(by,loy)+1;y++)
					p(x,y);
					op=8;break;
				default:break;
			}}else{if(y<tilesize*tilenum) // menu mode. figure out where pointer is and execute
				switch(x/tilesize*tilenum+y/tilesize){
					case 0:clear();if(op==3)op=2;break; // clear XXX maybe some more of those?
					case 1:canvas=loadimg(&height,&width,canvas);clipboard=(short*)malloc(height*width*sizeof(short));printf("%i %i\n",height,width);break; // load XXX also maybe check whether we even need to bother w/ rescaling selection (thatd require caching old sizes)
					case 2:op=0;break; // pencil
					case 3:op=2;bx=by=0;break; // line
					case 4:op=5;break; // bucket
					case 5:op=6;bx=by=0;;break; // rect
					case 6:if(psc<(1<<3))psc*=2;break; // zoomin XXX center (coords += (effective_size)/2)

					case tilenum+0:exit(0);break; // quit
					case tilenum+1:saveimg(height,width,canvas);break; // save
					case tilenum+2:gridon=gridon?false:true;break; // grid switch
					case tilenum+3:op=4;break; // pick
					case tilenum+4:op=1;break; // spray
					//case tilenum+5:op=8;bx=by=0;break; // full_rect
					case tilenum+5:op=(op==10)?11:10;break;
					case tilenum+6:if(psc>1)psc/=2;break; // zoomout XXX center
					default:break;
				}
			}
		}else // XXX do we need this else?

		if(GetMouse(0).bHeld){
			if(GetKey(olc::CTRL).bHeld){
				rin=canvas[lox*width+loy];
				printf("%03x (%03x) (%03x)\n",rin,hsv(rin),rgb(hsv(rin)));
			}else
			if(x>tilesize*2){switch(op){
				case 0:
					//p(lox,loy);
					drawline(lox,loy,bx,by);
					bx=lox;by=loy;
					break;
				case 1:spray(lox,loy);break;
			default:break;}
			}

			if(x<tilesize*2 && y>tilesize*tilenum){ // color sliders
				// this code is terrible and i know it but its five in the morning rn so i aint gonna change it
				char ty=(char)(16-((float)y-tilenum*tilesize)/(sw-tilenum*tilesize)*16);
				if(x<tilesize/2  ); // ignore
				else if(x<tilesize    ){rin = rin&0x0ff | ty<<8;} // select red
				else if(x<tilesize*3/2){rin = rin&0xf0f | ty<<4;} // select green
				else if(x<tilesize*2  ){rin = rin&0xff0 | ty<<0;} // select blue
			}
		}



		// XXX include some maximum border offset
		int inc=1;
		if(GetKey(olc::CTRL).bHeld)inc=3;
		if(GetKey(olc::UP   ).bHeld)coy+=inc;
		if(GetKey(olc::DOWN ).bHeld)coy-=inc;
		if(GetKey(olc::LEFT ).bHeld)cox+=inc;
		if(GetKey(olc::RIGHT).bHeld)cox-=inc;
		if(GetKey(olc::L).bPressed){cox=coy=0;psc=1;}

		if(GetKey(olc::M).bPressed)saveimg(height,width,canvas);
		if(GetKey(olc::O).bPressed)canvas=loadimg(&height,&width,canvas);

		if(GetKey(olc::G).bPressed)gridon=gridon?0:1;
		if(GetKey(olc::B).bPressed)op=0;
		if(GetKey(olc::W).bPressed)flip();
		if(GetKey(olc::R).bPressed && psc<(1<<4))psc*=2; //,cox-=width/2,coy-=height/2;
		if(GetKey(olc::F).bPressed && psc>1)psc/=2; //,cox+=width/2,coy+=height/2; // XXX twill take a bit more than that
		if(GetKey(olc::Q).bPressed || GetKey(olc::ESCAPE).bPressed)exit(0); // saveimg?

		return(render(x,y));
	}
};

int main(){
	//rin=0x0207;

	//canvas = (short*)malloc(height*width*sizeof(short));
	//clipboard = (short*)malloc(height*width*sizeof(short));
	//clear();
	canvas = loadimg(&height,&width,canvas);

	FILE*f=fopen("tiles.ppm","r");
	for(int i=0;i<15;i++)getc(f); // ignore pesky header, since we hardcode file size
	for(int i=0;i<128*256;i++)
	tiles[i]=
		(getc(f)>>4)<<8|
		(getc(f)>>4)<<4|
		(getc(f)>>4)<<0; // fancy!
	fclose(f);
	// also why tf am i not using the load function?!?
	// ah, right. different color spaces.
	// XXX not anymore - theres a blanket!
	//  load into canvas w/ loadimg, memcpy to tiles, clear canvas?
	//  wed have to reinitialise the canvas completely to make sure the size is right . . .

	View v;if(v.Construct(sh,sw,ss,ss))v.Start();

	return(0);
}


/*
filled rectangle doesnt work???

palette?
	might require us to rewrite how we handle colors
	or at lest add some more complicated functions

XXX height and width are still swapped! how?!?

filename?

grid scale selection

ye, moving sections does seem kinda vital
	two parts: making selection, and moving it
	rectangular or irregular selections? rectangulars are easier to make, but not that powerful
	wed need a second layer to store selection
		and we would finally have a use for transparency!
	we could select modes in menu (see note above abt keyboard shortcuts)
		in selection mode we "paint" over which parts we want selected (writes to selection alpha)
			how will we denote it w/out obstructing view of painting?
		in move mode, we cache the mouse position each press, and move the selection to mouse_pos-cached_pos while it is held
	confirming move would be required in some way, otherwise wed stamp it over every frame and thatd fuck up the drawing
	we also definitely have to have it render the layer as an overlay


k so
a clipboard
two more buttons (plus respective modes)
when first mode selected, we copy over canvas to clipboard
then when we click on it, it toggles whether pixels are selected (clipboard[x*w+y]|=0x1000)
then when clicked again, it puts us in paste mode
foreach in clipboard
	if selected, overwrite that pixel on canvas w/ current color
selection follows mouse
when clicked
	foreach in clipboard+mouse
		if selected, overwrite canvas w/ that color
	go over to paint mode

well have to edit how canvas is displayed
	if in selection mode paint canvas pixel then if pixel selected denote smh
	if in placement mode paint canvas then if selected paint clipboard then smh check if edge and denote
maybe do the thing with the checkers? since were usually zoomed in anyway

also couldja finally implement button shadows n shit? we need to see what mode were in

spray size

XXX idea: press tab to make interface go away

fix line/rectangle preview

ellipses

definitely spray size/density selection

add the upscaler project to this one?

either implement clipboard, or remove code for it

heyyy, remember that brush scale thing we have in chunker? yeeeah... that'd be nice.

hsv pickers

XXX we have some serious scaling/shifting issues

*/
