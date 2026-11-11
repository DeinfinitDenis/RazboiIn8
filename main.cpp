#include <GLFW/glfw3.h>
#include "text_renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

using namespace std;

// Coordonate: a1 = (0,0). Alb = 1, negru = -1.
struct Move { int from, to; };
struct Game {
    array<int,64> board{};
    int turn=1, ply=0;
    Game() {
        for(int y=0;y<8;++y) for(int x=0;x<8;++x)
            if((x+y)%2==0 && (y<2 || y>5)) board[y*8+x]=y<2?1:-1;
    }
    vector<int> destinations(int p) const {
        vector<int> result;
        for(int dx:{-1,1}) for(int dy:{-1,1}) {
            int x=p%8+dx,y=p/8+dy;
            if(x>=0&&x<8&&y>=0&&y<8&&!board[y*8+x]) result.push_back(y*8+x);
        }
        return result;
    }
    //minecraft
    vector<Move> moves() const {
        vector<Move> result;
        for(int p=0;p<64;++p) if(board[p]==turn)
            for(int q:destinations(p)) result.push_back({p,q});
        return result;
    }
    int count(int side) const { return int(::count(board.begin(),board.end(),side)); }
    bool over() const { return ply>=200 || !count(1) || !count(-1) || moves().empty(); }
    void apply(Move m) {
        board[m.to]=board[m.from]; board[m.from]=0; ++ply;
        // Primele patru mutari ale fiecaruia sunt de dezvoltare.
        // Evaluare simultana: eliminarea unei piese nu salveaza alta piesa blocata.
        if(ply>=8) {
            vector<int> captured;
            for(int p=0;p<64;++p) if(board[p]&&destinations(p).empty()) captured.push_back(p);
            for(int p:captured) board[p]=0;
        }
        turn=-turn;
    }
};
int evaluation(const Game& g) {
    int value=1000*(g.count(1)-g.count(-1));
    for(int p=0;p<64;++p) if(g.board[p])
        value+=g.board[p]*(int(g.destinations(p).size())*8+int(7-abs(2*(p%8)-7)));
    return value;
}
int search(const Game& g,int depth,int alpha,int beta) {
    if(!depth||g.over()) return evaluation(g);
    int best=g.turn==1?-1000000:1000000;
    for(auto m:g.moves()) {
        Game next=g; next.apply(m); int v=search(next,depth-1,alpha,beta);
        if(g.turn==1) { best=max(best,v); alpha=max(alpha,best); }
        else { best=min(best,v); beta=min(beta,best); }
        if(beta<=alpha) break;
    }
    return best;
}
Move computerMove(const Game& g) {
    auto list=g.moves(); Move best=list.front(); int value=1000000;
    for(auto m:list) { Game next=g; next.apply(m); int v=search(next,2,-1000000,1000000);
        if(v<value) { value=v; best=m; }
    }
    return best;
}
void color(float r,float g,float b) { glColor3f(r,g,b); }
void rect(float x,float y,float w,float h) {
    glBegin(GL_QUADS); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
}
struct TextTexture { GLuint id; int width,height; };
map<string,TextTexture> textCache;
// Render the bundled Noto Sans TrueType font, including Romanian diacritics.
void text(float x,float y,const string& s,float scale=2,float maxWidth=10000) {
    if(s.empty()) return;
    auto found=textCache.find(s);
    if(found==textCache.end()) {

        auto bitmap=rasterizeText(s);
        GLuint id; glGenTextures(1,&id); glBindTexture(GL_TEXTURE_2D,id);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,bitmap.width,bitmap.height,0,GL_ALPHA,GL_UNSIGNED_BYTE,bitmap.pixels.data());
        found=textCache.emplace(s,TextTexture{id,bitmap.width,bitmap.height}).first;
    }
    auto t=found->second; float factor=scale*.4f;
    factor=min(factor,maxWidth/t.width);
    float w=t.width*factor,h=t.height*factor;
    glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D,t.id);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(x,y); glTexCoord2f(1,0); glVertex2f(x+w,y);
    glTexCoord2f(1,1); glVertex2f(x+w,y+h); glTexCoord2f(0,1); glVertex2f(x,y+h);
    glEnd(); glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND);
}
void circle(float x,float y,float radius) {
    glBegin(GL_TRIANGLE_FAN); glVertex2f(x,y);
    for(int i=0;i<=64;++i) { float a=float(i)*6.2831853f/64; glVertex2f(x+radius*cos(a),y+radius*sin(a)); } glEnd();
}
enum class Screen { Menu, Settings, Playing };
enum class Language { Romanian, English };
struct Button { float x,y,w,h; int action; };
struct App {
    Screen screen=Screen::Menu, settingsReturn=Screen::Menu;
    Language language=Language::Romanian;
    vector<Button> buttons;
    bool quit=false;
    string tr(const char* ro,const char* en) const {
        return language==Language::Romanian?ro:en;
    }
    void button(float x,float y,float w,const string& label,int action,bool active=false) {
        buttons.push_back({x,y,w,48,action});
        if(active) color(.32f,.47f,.40f); else color(.14f,.23f,.28f);
        rect(x,y,w,48); color(.93f,.94f,.90f);
        text(x+16,y+8,label,1.8f,w-32);
    }
    void action(int id) {
        if(id==1||id==2) { ai=id==2; reset(); help=false; screen=Screen::Playing; }
        if(id==3) { settingsReturn=screen; screen=Screen::Settings; }
        if(id==4) quit=true;
        if(id==5) screen=settingsReturn;
        if(id>=10&&id<=11) language=static_cast<Language>(id-10);
        if(id==6) { screen=Screen::Menu; help=false; }
        if(id==7) screen=Screen::Playing;
    }
    void key(int key) {
        if(key==GLFW_KEY_ESCAPE) {
            if(screen==Screen::Settings) screen=settingsReturn;
            else if(screen==Screen::Playing) { if(help) help=false; else screen=Screen::Menu; }
            else quit=true;
            return;
        }
        if(screen!=Screen::Playing) return;
        if(key==GLFW_KEY_R) reset();
        if(key==GLFW_KEY_U) undo();
        if(key==GLFW_KEY_H) help=!help;
        if(key==GLFW_KEY_C) { screen=Screen::Menu; help=false; }
    }
    Game game;
    vector<Game> history;
    int selected=-1;
    bool ai=false,help=false;
    float bx=40,by=100,cell=70;
    double aiAt=0;
    void reset() { game=Game{}; history.clear(); selected=-1; aiAt=glfwGetTime()+0.5; }
    void play(Move m) { history.push_back(game); game.apply(m); selected=-1; aiAt=glfwGetTime()+0.45; }
    void undo() {
        if(history.empty()) return;
        game=history.back(); history.pop_back();
        if(ai&&game.turn==-1&&!history.empty()) { game=history.back(); history.pop_back(); }
        selected=-1; aiAt=glfwGetTime()+0.5;
    }
    void click(double x,double y) {
        for(auto b:buttons) if(x>=b.x&&x<=b.x+b.w&&y>=b.y&&y<=b.y+b.h) { action(b.action); return; }
        if(screen!=Screen::Playing) return;
        if(help||game.over()||(ai&&game.turn==-1)) return;
        int cx=int(floor((x-bx)/cell)),cy=int(floor((y-by)/cell));
        if(cx<0||cx>=8||cy<0||cy>=8) { selected=-1; return; }
        int p=(7-cy)*8+cx;
        if(game.board[p]==game.turn) { selected=p; return; }
        if(selected>=0) { auto options=game.destinations(selected);
            if(find(options.begin(),options.end(),p)!=options.end()) play({selected,p});
            else selected=-1;
        }
    }
    void draw(int w,int h) {
        buttons.clear();
        if(screen!=Screen::Playing) {
            float x=w/2.f-220;
            color(.94f,.78f,.42f);
            text(x,65,tr("Război în 8","War in Eight"),3.5f,440);
            color(.75f,.82f,.86f);
            if(screen==Screen::Menu) {
                text(x,135,tr("Meniu principal","Main menu"),2,440);
                button(x,205,440,tr("Jucător vs Jucător","Player vs Player"),1);
                button(x,268,440,tr("Jucător vs Calculator","Player vs Computer"),2);
                button(x,331,440,tr("Setări","Settings"),3);
                button(x,394,440,tr("Ieșire","Exit"),4);
                if(!history.empty()) button(x,457,440,tr("Continuă partida","Resume game"),7);
            } else {
                text(x,135,tr("Setări - Limbă","Settings - Language"),2,440);
                button(x,205,440,"Română",10,language==Language::Romanian);
                button(x,268,440,"English",11,language==Language::English);
                button(x,350,440,tr("Înapoi","Back"),5);
            }
            return;
        }
        cell=max(1.f,min((w-360.f)/8,(h-150.f)/8)); bx=40; by=100;
        color(.89f,.92f,.95f); text(40,22,tr("Război în 8","War in Eight"),3);
        text(40,65,tr("Olimpiada Jocurilor Raționale","Olympiad of Rational Games"),1.5f);
        auto options=selected<0?vector<int>{}:game.destinations(selected);
        for(int p=0;p<64;++p) {
            float x=bx+(p%8)*cell,y=by+(7-p/8)*cell;
            if((p%8+p/8)%2==0) color(.18f,.28f,.32f); else color(.67f,.73f,.70f);
            rect(x,y,cell,cell);
            if(p==selected) { color(.87f,.65f,.23f); rect(x+2,y+2,cell-4,cell-4); }
            if(find(options.begin(),options.end(),p)!=options.end()) { color(.30f,.85f,.65f); circle(x+cell/2,y+cell/2,cell*.12f); }
            if(game.board[p]) {
                color(.05f,.08f,.10f); circle(x+cell*.52f,y+cell*.55f,cell*.35f);
                if(game.board[p]==1) color(.94f,.90f,.76f); else color(.12f,.16f,.21f);
                circle(x+cell/2,y+cell/2,cell*.34f);
                if(game.board[p]==1) color(.69f,.64f,.49f); else color(.33f,.43f,.51f);
                circle(x+cell/2,y+cell/2,cell*.24f);
                if(game.board[p]==1) color(.94f,.90f,.76f); else color(.12f,.16f,.21f);
                circle(x+cell/2,y+cell/2,cell*.20f);
            }
        }
        color(.75f,.82f,.86f);
        for(int i=0;i<8;++i) { text(bx+i*cell+cell/2-4,by+8*cell+12,string(1,char('A'+i)),1.5f);
            text(20,by+(7-i)*cell+cell/2-5,to_string(i+1),1.5f); }
        float sx=bx+8*cell+28, yy=105;
        auto line=[&](const string& s) { text(sx,yy,s,1.5f,w-sx-20); yy+=28; };
        color(.94f,.78f,.42f);
        line(ai?tr("Contra calculatorului","Against computer"):tr("Doi jucători","Two players")); yy+=12;
        color(.89f,.92f,.95f);
        if(game.over()) {
            int score=game.count(1)-game.count(-1);
            line(score>0?tr("Victorie alb","White wins"):score<0?tr("Victorie negru","Black wins"):tr("Remiză","Draw"));
        } else line(game.turn==1?tr("La mutare - Alb","White to move"):tr("La mutare - Negru","Black to move"));
        line(tr("Alb: ","White: ")+to_string(game.count(1)));
        line(tr("Negru: ","Black: ")+to_string(game.count(-1)));
        line(tr("Tura: ","Round: ")+to_string(min(100,game.ply/2+1))+" / 100"); yy+=12;
        color(.44f,.84f,.71f);
        line(game.ply<8?tr("Faza de dezvoltare","Opening phase"):tr("Eliminări active","Captures active"));
        if(game.ply<8) line(tr("Pași rămași: ","Steps left: ")+to_string(8-game.ply));
        yy+=12; color(.75f,.82f,.86f);
        line(tr("Click - selectează și mută","Click - select and move"));
        line(tr("U - Anulează","U - Undo"));
        line(tr("R - Joc nou","R - New game"));
        line(tr("H - Reguli","H - Rules"));
        line(tr("Esc - Meniu principal","Esc - Main menu"));
        button(sx,yy+10,w-sx-20,tr("Meniu principal","Main menu"),6);
        button(sx,yy+68,w-sx-20,tr("Setări","Settings"),3);
        if(help) {
            buttons.clear();
            color(.055f,.09f,.12f); rect(45,115,float(w-90),450);
            color(.94f,.78f,.42f); text(70,130,tr("Reguli","Rules"),2.5f,w-140);
            color(.9f,.93f,.95f);
            vector<string> lines={
                tr("Fiecare jucător are 8 piese. Albul începe.","Each player has 8 pieces. White starts."),
                tr("Mută un pas pe o diagonală liberă, în orice sens.","Move one step to a free diagonal square in any direction."),
                tr("Nu sunt permise săriturile.","Jumping is not allowed."),
                tr("Primele 4 mutări ale fiecăruia sunt de dezvoltare.","Each player gets 4 opening moves before captures start."),
                tr("Apoi piesele fără ieșire diagonală sunt eliminate.","Then pieces without a free diagonal exit are removed."),
                tr("Eliminările sunt automate și simultane, pentru ambele culori.","Captures are automatic and simultaneous for both sides."),
                tr("Cel mult 100 de mutări de fiecare. Câștigă scorul mai mare.","Up to 100 moves each. The side with more pieces wins."),
                tr("La același număr de piese, partida se încheie remiză.","Equal remaining pieces means a draw."),
                tr("Apasă H sau Esc pentru a reveni.","Press H or Esc to return.")};
            float y=195; for(const auto& label:lines) { text(70,y,label,1.7f,w-140); y+=36; }
        }
    }
};
void check(bool b,const char* message) { if(!b) throw runtime_error(message); }
void selfTest() {
    for(int code:{0x0103,0x00e2,0x00ee,0x0219,0x021b,0x0102,0x00c2,0x00ce,0x0218,0x021a})
        check(fontHasGlyph(code),"Romanian font glyph");
    auto label=rasterizeText("Război în 8 — ăâîșț ĂÂÎȘȚ");
    check(label.width>0 && label.height>0 && any_of(label.pixels.begin(),label.pixels.end(),[](unsigned char p){ return p>0; }),"TrueType text rendering");
    Game g; check(g.count(1)==8&&g.count(-1)==8,"initial pieces");
    check(g.destinations(0).empty(),"initial blocked corner");
    auto moves=g.moves(); check(moves.size()==7,"initial legal moves");
    g.apply({9,16}); check(g.count(1)==8&&g.count(-1)==8,"opening immunity");
    g.board.fill(0); g.board[0]=-1; g.board[18]=1; g.ply=8; g.turn=1;
    g.apply({18,9}); check(g.board[0]==0&&g.count(-1)==0,"corner capture");
    g.board.fill(0); g.board[27]=-1; for(int p:{18,20,34}) g.board[p]=1;
    g.board[45]=1; g.ply=10; g.turn=1; g.apply({45,36});
    check(g.board[27]==0,"four diagonal blockade");
    g=Game{}; g.ply=199; g.apply(g.moves().front()); check(g.over(),"move limit");
    App app;
    check(app.screen==Screen::Menu,"starts in main menu");
    app.action(3); check(app.screen==Screen::Settings,"open settings");
    app.action(11); check(app.tr("ro","en")=="en","English translation selection");
    app.action(10); check(app.tr("ro","en")=="ro","Romanian translation selection");
    app.key(GLFW_KEY_ESCAPE); check(app.screen==Screen::Menu,"settings back");
    app.screen=Screen::Playing; app.game=g;
    app.action(3); app.action(11); app.action(5);
    check(app.screen==Screen::Playing&&app.game.ply==200,"settings preserve game");
    check(app.tr("ro","en")=="en","English translation selection");
    app.help=true; app.key(GLFW_KEY_ESCAPE);
    check(!app.help&&app.screen==Screen::Playing,"close rules before leaving game");
    app.key(GLFW_KEY_ESCAPE); check(app.screen==Screen::Menu,"return to main menu");
    cout<<"Testele regulilor si navigarii au trecut.\n";
}
int main(int argc,char** argv) {
    try { loadFont(argv[0]); }
    catch(const exception& e) { cerr<<e.what()<<'\n'; return 1; }
    if(argc>1&&string(argv[1])=="--self-test") { try { selfTest(); return 0; }
        catch(const exception& e) { cerr<<e.what()<<'\n'; return 1; } }
    glfwSetErrorCallback([](int,const char* message){ cerr<<"GLFW: "<<message<<'\n'; });
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    GLFWwindow* window=glfwCreateWindow(1060,760,"Razboi in 8",nullptr,nullptr);
    if(!window) { glfwTerminate(); return 1; }
    glfwSetWindowSizeLimits(window,920,650,GLFW_DONT_CARE,GLFW_DONT_CARE);
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    App app; glfwSetWindowUserPointer(window,&app);
    glfwSetMouseButtonCallback(window,[](GLFWwindow* win,int button,int action,int){
        if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS) { double x,y; glfwGetCursorPos(win,&x,&y);
            static_cast<App*>(glfwGetWindowUserPointer(win))->click(x,y); }
    });
    glfwSetKeyCallback(window,[](GLFWwindow* win,int key,int,int action,int){
        if(action!=GLFW_PRESS) return;
        auto& a=*static_cast<App*>(glfwGetWindowUserPointer(win));
        a.key(key);
    });
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if(app.screen==Screen::Playing&&app.ai&&!app.help&&app.game.turn==-1&&!app.game.over()&&glfwGetTime()>=app.aiAt) app.play(computerMove(app.game));
        if(app.quit) break;
        int w,h,fw,fh; glfwGetWindowSize(window,&w,&h); glfwGetFramebufferSize(window,&fw,&fh);
        glViewport(0,0,fw,fh); glClearColor(.055f,.09f,.12f,1); glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,w,h,0,-1,1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity(); app.draw(w,h); glfwSwapBuffers(window);
    }
    for(const auto& entry:textCache) glDeleteTextures(1,&entry.second.id);
    textCache.clear();
    glfwDestroyWindow(window); glfwTerminate();
}
