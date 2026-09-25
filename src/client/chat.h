#ifndef CODARIS_CHAT_H
#define CODARIS_CHAT_H
#include "../shared/chat_preview.h"
static ChatState chat;
EM_JS(void, chat_option, (int index,const char *name), {
 const select=document.getElementById('chat-country');if(!select)return;
 const option=document.createElement('option');option.value=String(index);option.textContent=UTF8ToString(name);select.append(option);
})
EM_JS(void, chat_view_room, (const char *name,int room), { if (window.CodarisChatView) window.CodarisChatView.room(UTF8ToString(name),room); })
EM_JS(void, chat_view_row, (int id,int actor,const char *name,const char *text,const char *file,int reported), { if (window.CodarisChatView) window.CodarisChatView.row(id,actor,UTF8ToString(name),UTF8ToString(text),UTF8ToString(file),!!reported); })
EM_JS(void, chat_view_sent, (int id), { if (window.CodarisChatView) window.CodarisChatView.sent(id); })
EM_JS(void, chat_view_report, (int actor), { if (window.CodarisChatView) window.CodarisChatView.report(actor); })
EM_JS(void, chat_view_blocked, (int first,int second), { if (window.CodarisChatView) window.CodarisChatView.blocked(first,second); })
static void chat_render(void) {
 chat_view_room(chat.room<0?"Choose a country":chat_countries[chat.room],chat.room);
 if(chat.room<0)return;
 if(!chat.blocked[1])chat_view_row(-1,1,"Alex · example","Welcome! What are you building or learning this week?","",chat.reported[1]);
 if(!chat.blocked[2])chat_view_row(-2,2,"Sam · example","A useful debugging habit: reduce the problem to a small, reproducible example.","",chat.reported[2]);
 for(int i=0;i<chat.count;++i)if(chat.messages[i].room==chat.room){ChatMessage *m=&chat.messages[i];chat_view_row(i,0,m->name,m->text,m->file,0);}
 chat_view_blocked(chat.blocked[1],chat.blocked[2]);
}
static void chat_init(void) {
 chat_reset(&chat);
 if(!member_development())return;
 for(int i=0;i<50;++i)chat_option(i,chat_countries[i]);
 chat_render();
}
EMSCRIPTEN_KEEPALIVE void codaris_chat_join(int room) {
 if(!member_development())return;
 if(room<0||room>=50){view_text("chat-feedback","Choose one of the 50 country rooms.");return;}
 chat.room=room;chat.report_target=0;chat_view_report(0);chat_render();
 view_text("chat-feedback","Room entered. Example members are fictional; messages stay on this page.");
}
EMSCRIPTEN_KEEPALIVE void codaris_chat_leave(void) {
 if(!member_development())return;
 chat.room=-1;chat.report_target=0;chat_view_report(0);chat_render();view_text("chat-feedback","Choose a country to enter a room. Your local room history remains until reload.");
}
EMSCRIPTEN_KEEPALIVE int codaris_chat_file(const char *name,const char *mime,double size) {
 if(!member_development()||chat.room<0)return 0;
 const char *error=chat_attachment(name,mime,size);view_text("chat-feedback",error?error:"Attachment selected locally. Nothing has been uploaded.");return error==NULL;
}
EMSCRIPTEN_KEEPALIVE void codaris_chat_send(const char *name,const char *text,const char *file,double now) {
 if(!member_development())return;
 const char *error=chat_send(&chat,name,text,file,now);
 if(error){view_text("chat-feedback",error);return;}
 chat_view_sent(chat.count-1);chat_render();view_text("chat-feedback","Message added to this room preview. Not shared or saved.");
}
EMSCRIPTEN_KEEPALIVE void codaris_chat_action(int actor,int action) {
 if(!member_development()||chat.room<0||actor<1||actor>2)return;
 if(action==0){chat.report_target=actor;chat_view_report(actor);return;}
 if(action==1||action==2){chat.blocked[actor]=action==1;chat_render();view_text("chat-feedback",action==1?"Example member blocked across this preview. Their messages are hidden.":"Example member unblocked.");}
}
EMSCRIPTEN_KEEPALIVE void codaris_chat_report(int reason) {
 if(!member_development()||chat.room<0||!chat.report_target)return;
 if(reason==-1){chat.report_target=0;chat_view_report(0);return;}
 if(reason<1||reason>4){view_text("chat-feedback","Choose a reason for the report.");return;}
 chat.reported[chat.report_target]=1;chat.report_target=0;chat_view_report(0);chat_render();view_text("chat-feedback","Report marked in this preview only. No moderator received it.");
}
#endif
