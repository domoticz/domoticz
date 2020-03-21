#include<simplecpp>
#include<ctime>
void show(int x,int y,int text[12][12])
{if(text[x][y]<0)
return;

    Rectangle r((x-1)*50+25,(y-1)*50+25,44,44);
    r.setFill(true);
    if(text[x][y]==4)
    r.setColor(COLOR(100,39,48));
    if(text[x][y]==3)
    r.setColor(COLOR(200,50,99));
    if(text[x][y]==2)
    r.setColor(COLOR(25,192,64));
    if(text[x][y]==1)
    r.setColor(COLOR(98,27,199));
    if(text[x][y]==0)
    r.setColor(COLOR(193,193,129));

    r.imprint();
     Text t((x-1)*50+25,(y-1)*50+25,text[x][y]);
     t.imprint();


    if(text[x][y]>0)
    {text[x][y]=-6;return;}
     else
     {text[x][y]=-6;
         show(x+1,y,text);
         show(x+1,y+1,text);
         show(x+1,y-1,text);
         show(x-1,y,text);
         show(x-1,y+1,text);
         show(x-1,y-1,text);
         show(x,y+1,text);
         show(x,y-1,text);
     }
}
main()
{srand(time(0));
initCanvas("mkjo",500,500);
for(int i=0;i<=500;i+=50)
    {Line l1(i,0,i,500);
    Line l2(0,i,500,i);
  l2.imprint()  ;
l1.imprint();}

for(int i=25;i<500;i+=50)
{
    for(int j=25;j<500;j+=50)
    {
        Rectangle r(i,j,44,44);
        r.setFill(true);
        r.setColor(COLOR("green"));
        r.imprint();
    }
}
int text[12][12];
for(int i=1;i<11;i++)
{
    for(int j=1;j<11;j++)
        text[i][j]=0;
}

    for(int i=0;i<12;i++)
    {
        text[0][i]=-10;text[i][0]=-10;text[11][i]=-10;text[i][11]=-10;
    }
repeat(20)
{
    int i=randuv(1,11);
    int j=randuv(1,11);
    text[i][j]=-1;
}


for(int i=1;i<11;i++)
{
    for(int j=1;j<11;j++)
    {if(text[i][j]==-1)
    {
        continue;
    }else{
            if(text[i+1][j]==-1)
            text[i][j]++;
            if(text[i+1][j-1]==-1)
            text[i][j]++;
            if(text[i+1][j+1]==-1)
            text[i][j]++;
            if(text[i][j+1]==-1)
            text[i][j]++;
            if(text[i][j-1]==-1)
            text[i][j]++;
            if(text[i-1][j+1]==-1)
            text[i][j]++;
            if(text[i-1][j-1]==-1)
            text[i][j]++;
            if(text[i-1][j]==-1)
            text[i][j]++;
    }
    }}

long long i=getClick();
int x=(i/65536)/50+1;int y=(i%65536)/50+1;int k=0;
while(text[x][y]!=-1&&k!=100)
{if(text[x][y]==-6)
continue;


    Rectangle r((x-1)*50+25,(y-1)*50+25,44,44);
    r.setFill(true);
    if(text[x][y]==4)
    r.setColor(COLOR(100,39,48));
    if(text[x][y]==3)
    r.setColor(COLOR(200,50,99));
    if(text[x][y]==2)
    r.setColor(COLOR(25,192,64));
    if(text[x][y]==1)
    r.setColor(COLOR(98,27,199));
    if(text[x][y]==0)
    r.setColor(COLOR(193,193,129));

    r.imprint();
     Text t((x-1)*50+25,(y-1)*50+25,text[x][y]);
     t.imprint();

    if(text[x][y]==0)

     {
         show(x+1,y,text);
         show(x+1,y+1,text);
         show(x+1,y-1,text);
         show(x-1,y,text);
         show(x-1,y+1,text);
         show(x-1,y-1,text);
         show(x,y+1,text);
         show(x,y-1,text);
     }text[x][y]=-6;
    i=getClick();
x=(i/65536)/50+1; y=(i%65536)/50+1;k=0;
for(int i=1;i<11;i++)
{
    for(int j=1;j<11;j++)
    {
        if(text[i][j]<0)
            k++;
    }
}
}


if(text[x][y]==-1)
{for(int i=1;i<11;i++)
{
    for(int j=1;j<11;j++)
    {
        if(text[i][j]==-1)
           {
                          Rectangle r((i-1)*50+25,(j-1)*50+25,44,44);
            r.setFill(true);
                r.setColor(COLOR(100,39,48));     Text t((i-1)*50+25,(j-1)*50+25,"bomb");    r.imprint();     t.imprint();}


    }}

    Rectangle r(250,250,500,500);
    r.setFill(true);repeat(10){
    r.setColor(COLOR("red"));
        r.setColor(COLOR("black"));
        r.setColor(COLOR("red"));
        r.setColor(COLOR("black"));
        r.setColor(COLOR("red"));
        r.setColor(COLOR("black"));}
            r.setColor(COLOR("red"));

        Text t(250,250,"game Over");
        for(double i=1;i<4;i+=0.1)
{

         t.scale(i);
        t.imprint();
}        wait(15);


}
else
{Rectangle r(250,250,500,500);
    r.setFill(true);repeat(10){
    r.setColor(COLOR("green"));
        r.setColor(COLOR("orange"));}

        Text t(250,250,"congratulations");

        t.setFill(true);
        t.setColor(COLOR("black"));
    t.imprint();r.imprint();

}}

