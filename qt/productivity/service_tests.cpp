#include "service.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <iostream>
#include <functional>
#include <QSet>
#include <QFileInfo>
using namespace pets::productivity;
static int checks = 0;
static void check(bool ok) { ++checks; if (!ok) throw std::runtime_error("check " + std::to_string(checks)); }
static void fails(int code, const std::function<void()> &fn) {
    try { fn(); } catch (const Error &e) { check(e.code == code); return; }
    throw std::runtime_error("expected error " + std::to_string(code));
}
static QJsonObject event(int seq=1, QString kind="needs_input", QString id="one") {
    return {{"source_id","codex"},{"event_id",id},{"seq",seq},{"kind",kind},{"title","Needs your input"}};
}
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);
 try {
    QTemporaryDir dir; check(dir.isValid()); QString path=dir.filePath("state.json");
    Service s(path); check(s.error().isEmpty());
    auto item=s.call("publish",event(),1000)["item"].toObject(); check(item["created_at"]==1000);
    Service restored(path); check(restored.call("list",{},1000)["total"]==1);
    Service occurrences(dir.filePath("occurrences.json"));auto occurrence=event();occurrence["occurrence_id"]="request-1";
    occurrences.call("publish",occurrence,0);
    occurrences.call("act",{{"source_id","codex"},{"event_id","one"},{"seq",1},{"action","acknowledge"}},0);
    occurrence["seq"]=2;check(occurrences.call("publish",occurrence,1)["item"].toObject()["acknowledged"].toBool());
    occurrence["seq"]=3;occurrence["occurrence_id"]="request-2";
    auto occurrenceItem=occurrences.call("publish",occurrence,2)["item"].toObject();
    check(!occurrenceItem["acknowledged"].toBool() && occurrenceItem["occurrence_id"]=="request-2");
    Service restoredOccurrences(dir.filePath("occurrences.json"));check(restoredOccurrences.error().isEmpty());
    check(restoredOccurrences.call("list",{},2)["items"].toArray()[0].toObject()["occurrence_id"]=="request-2");
    occurrences.call("act",{{"source_id","codex"},{"event_id","one"},{"seq",3},{"action","complete"}},2);
    occurrence["seq"]=4;occurrence["occurrence_id"]="request-3";
    check(!occurrences.call("publish",occurrence,3)["item"].toObject()["completed"].toBool());
    occurrences.call("act",{{"source_id","codex"},{"event_id","one"},{"seq",4},{"action","snooze"}},3);
    occurrence["seq"]=5;occurrence["occurrence_id"]="request-4";
    check(occurrences.call("publish",occurrence,4)["item"].toObject()["snoozed_until"]==0);
    auto noOccurrence=event(6);check(!occurrences.call("publish",noOccurrence,5)["item"].toObject().contains("occurrence_id"));
    occurrence["seq"]=7;occurrence["occurrence_id"]="bad occurrence";fails(-32602,[&]{occurrences.call("publish",occurrence,6);});
    fails(-32003,[&]{s.call("publish",event(),1100);});
    auto action=QJsonObject{{"source_id","codex"},{"event_id","one"},{"seq",1},{"action","acknowledge"}};
    s.call("act",action,1100); check(s.call("list",{{"attention_only",true}},1100)["total"]==0);
    s.call("publish",event(2),1200); check(s.call("list",{},1200)["items"].toArray()[0].toObject()["acknowledged"].toBool());
    s.call("publish",event(3,"failed"),1300); check(s.call("list",{{"attention_only",true}},1300)["total"]==1);
    action["seq"]=3; action["action"]="snooze"; action["seconds"]=2;
    s.call("act",action,1400); check(s.nextDeadline()==3400); s.advance(3400);
    check(s.call("list",{{"attention_only",true}},3400)["total"]==1); check(!s.nextDeadline());
    action["action"]="complete"; s.call("act",action,3500);
    item=s.call("list",{},3500)["items"].toArray()[0].toObject(); check(item["completed"].toBool() && !item["acknowledged"].toBool());
    auto timer=s.call("timer_create",{{"label","Tea"},{"seconds",2}},4000)["timer"].toObject(); QString id=timer["id"].toString();
    s.call("timer_act",{{"id",id},{"action","pause"}},4500); check(!s.nextDeadline());
    Service paused(path); check(paused.call("timers",{},9000)["timers"].toArray()[0].toObject()["remaining_ms"]==1500);
    paused.call("timer_act",{{"id",id},{"action","resume"}},9000); paused.advance(10500);
    check(paused.call("timers",{},10500)["timers"].toArray()[0].toObject()["state"]=="finished");
    check(paused.call("list",{{"attention_only",true}},10500)["total"]==1); paused.advance(999999);
    check(paused.call("list",{{"attention_only",true}},999999)["total"]==1);
    Service pom(dir.filePath("pom.json"));
    id=pom.call("timer_create",{{"label","Focus"},{"seconds",1},{"mode","pomodoro"},{"break_seconds",2},{"long_break_seconds",3},{"rounds",2}},0)["timer"].toObject()["id"].toString();
    pom.advance(100000); check(!pom.nextDeadline());
    timer=pom.call("timer_act",{{"id",id},{"action","next"}},100000)["timer"].toObject(); check(timer["phase"]=="short_break" && timer["deadline"]==102000);
    pom.advance(102000); pom.call("timer_act",{{"id",id},{"action","next"}},102000); pom.advance(103000);
    timer=pom.call("timer_act",{{"id",id},{"action","next"}},103000)["timer"].toObject(); check(timer["phase"]=="long_break");
    Service full(dir.filePath("full.json"));
    for(int i=0;i<512;++i) full.call("publish",event(1,"info",QString::number(i)),i);
    fails(-32004,[&]{full.call("publish",event(1,"info","extra"),1000);});
    check(full.call("list",{{"offset",500},{"limit",50}},1000)["items"].toArray().size()==12);
    fails(-32602,[&]{full.call("list",{{"attention_only","true"}},1000);});
    fails(-32602,[&]{full.call("publish",event(1,"info","bad id"),1000);});
    auto bad=event(); bad["source_url"]="javascript:alert(1)"; fails(-32602,[&]{full.call("publish",bad,1000);});
    Service literal(dir.filePath("literal.json"));bad=event();bad["title"]="Build < 10 > 2";literal.call("publish",bad,1000);check(literal.call("list",{},1000)["total"]==1);
    fails(-32602,[&]{full.call("list",{},-1);});
    QFile corrupt(dir.filePath("bad.json")); check(corrupt.open(QIODevice::WriteOnly)); corrupt.write("{broken"); corrupt.close();
    Service broken(corrupt.fileName()); check(!broken.error().isEmpty()); fails(-32010,[&]{broken.call("publish",event(),0);});
    check(corrupt.open(QIODevice::ReadOnly)); check(corrupt.readAll()=="{broken"); corrupt.close();
    Service unwritable(dir.filePath("missing/state.json")); fails(-32010,[&]{unwritable.call("publish",event(),0);});
    check(unwritable.call("list",{},0)["total"]==0); QDir().mkdir(dir.filePath("missing")); unwritable.call("publish",event(),0); check(unwritable.error().isEmpty());
    Service wide(dir.filePath("wide.json")); bad=event(); bad["detail"]=QString(2048,'\\');
    for(int i=0;i<50;++i){bad["event_id"]=QString::number(i);wide.call("publish",bad,i);}
    auto page=wide.call("list",{},100); check(page["total"]==50); check(page["items"].toArray().size()>0 && QJsonDocument(page).toJson(QJsonDocument::Compact).size()<60000);
    // Mixed record sizes must paginate contiguously, even at the byte budget.
    Service mixed(dir.filePath("mixed.json"));
    for(int i=0;i<50;++i) {auto e=event(1,"info",QString::number(i));if(i>0)e["detail"]=QString(2048,QChar(92));mixed.call("publish",e,i);}
    auto first=mixed.call("list",{},100)["items"].toArray();auto second=mixed.call("list",{{"offset",first.size()}},100)["items"].toArray();
    QSet<QString> seen;for(auto v:first)seen.insert(v.toObject()["event_id"].toString());for(auto v:second)check(!seen.contains(v.toObject()["event_id"].toString()));
    auto reserved=event();reserved["source_id"]="timer";fails(-32602,[&]{mixed.call("publish",reserved,100);});
    // Failed commit rolls memory back and keeps the previous durable file intact.
    Service atomic(dir.filePath("atomic.json"));atomic.call("publish",event(),0);
    QFile before(dir.filePath("atomic.json"));check(before.open(QIODevice::ReadOnly));auto original=before.readAll();before.close();
    check(QFile::rename(before.fileName(),dir.filePath("previous.json")));check(QDir().mkdir(before.fileName()));
    fails(-32010,[&]{atomic.call("publish",event(2),10);});check(!atomic.nextDeadline());
    check(atomic.call("list",{},10)["items"].toArray()[0].toObject()["seq"]==1);
    check(QDir().rmdir(before.fileName()));check(QFile::rename(dir.filePath("previous.json"),before.fileName()));
    check(before.open(QIODevice::ReadOnly));check(before.readAll()==original);before.close();atomic.call("publish",event(2),10);
    // Restoring a syntactically valid but invalid model must also fail closed.
    QFile malformed(dir.filePath("malformed.json"));check(malformed.open(QIODevice::WriteOnly));malformed.write(R"({"version":1,"items":[{}],"timers":[]})");malformed.close();
    Service malformedStore(malformed.fileName());check(!malformedStore.error().isEmpty());
    Service reconcile(dir.filePath("reconcile.json"));
    reconcile.call("timer_create",{{"label","Due"},{"seconds",1}},0);
    check(reconcile.call("list",{},1000)["total"]==1);
    auto link=event(2);link["source_url"]="https://example.com/task";literal.call("publish",link,2000);literal.call("publish",event(3),2001);
    check(literal.call("list",{},2001)["items"].toArray()[0].toObject()["source_url"].toString().isEmpty());
    check(QFileInfo(dir.filePath("literal.json")).permissions()==(QFile::ReadOwner|QFile::WriteOwner|QFile::ReadUser|QFile::WriteUser));
    Service crowded(dir.filePath("crowded.json"));crowded.call("timer_create",{{"label","Overdue"},{"seconds",1}},0);
    for(int i=0;i<512;++i)crowded.call("publish",event(1,"info",QString::number(i)),0);
    // A repair must work on the first call at the deadline, before any failed advance.
    for(const auto &repair:QStringList{"acknowledge","complete","cancel"}) {
        const auto repairPath=dir.filePath(repair+".json");check(QFile::copy(dir.filePath("crowded.json"),repairPath));
        Service repairStore(repairPath);
        if(repair=="cancel") {
            QJsonValue timerId=repairStore.call("timers",{},0)["timers"].toArray()[0].toObject()["id"];
            check(repairStore.call("timer_act",{{"id",timerId},{"action","cancel"}},1000)["timer"].toObject()["state"]=="cancelled");
        } else {
            check(repairStore.call("act",{{"source_id","codex"},{"event_id","0"},{"seq",1},{"action",repair}},1000)["item"].toObject()[repair=="complete" ? "completed" : "acknowledged"].toBool());
        }
        check(repairStore.error().isEmpty());check(!repairStore.nextDeadline());
        Service durableRepair(repairPath);check(durableRepair.call("timers",{},1000)["timers"].toArray()[0].toObject()["state"]==(repair=="cancel" ? "cancelled" : "finished"));
    }
    // Unknown operations must not reconcile and write a timer as a side effect.
    const auto partialPath=dir.filePath("partial-repair.json");check(QFile::copy(dir.filePath("crowded.json"),partialPath));
    Service partial(partialPath);partial.call("timer_create",{{"label","Second due"},{"seconds",1}},0);
    check(partial.call("act",{{"source_id","codex"},{"event_id","0"},{"seq",1},{"action","acknowledge"}},1000)["item"].toObject()["acknowledged"].toBool());
    check(!partial.error().isEmpty());check(!partial.nextDeadline());
    Service durablePartial(partialPath);check(durablePartial.call("list",{{"attention_only",true}},0)["total"]==511);
    partial.call("act",{{"source_id","codex"},{"event_id","1"},{"seq",1},{"action","complete"}},1000);
    check(partial.error().isEmpty());check(!partial.nextDeadline());
    const auto deniedPath=dir.filePath("denied-repair.json");check(QFile::copy(dir.filePath("crowded.json"),deniedPath));
    Service denied(deniedPath);check(QFile::rename(deniedPath,deniedPath+".previous"));check(QDir().mkdir(deniedPath));
    fails(-32010,[&]{denied.call("act",{{"source_id","codex"},{"event_id","0"},{"seq",1},{"action","acknowledge"}},1000);});
    check(denied.call("list",{{"attention_only",true}},0)["total"]==512);
    Service unknown(dir.filePath("unknown.json"));unknown.call("timer_create",{{"label","Due"},{"seconds",1}},0);
    fails(-32601,[&]{unknown.call("not_an_operation",{},1000);});check(unknown.nextDeadline()==1000);
    fails(-32004,[&]{crowded.advance(1000);});check(!crowded.nextDeadline());
    crowded.call("act",{{"source_id","codex"},{"event_id","0"},{"seq",1},{"action","acknowledge"}},1001);crowded.advance(1001);check(crowded.error().isEmpty());
    full.call("act",{{"source_id","codex"},{"event_id","0"},{"seq",1},{"action","acknowledge"}},1000);
    full.call("publish",event(1,"info","replacement"),1000);check(full.call("list",{},1000)["total"]==512);
    check(full.call("list",{{"query","replacement"}},1000)["total"]==0); // Search is source/title/detail, not identity.
    for(auto value:{QJsonValue("2"),QJsonValue(1.5),QJsonValue(-1),QJsonValue(9007199254740992.0)}) {auto e=event(3);e["seq"]=value;fails(-32602,[&]{literal.call("publish",e,2001);});}
    for(auto value:{"https://user:secret@example.com","https://example.com/%0a","https://example.com/%zz","file://remote/path","relative/path"}) {auto e=event(3);e["source_url"]=value;fails(-32602,[&]{literal.call("publish",e,2001);});}
    Service maxTimers(dir.filePath("max-timers.json"));
    for(int i=0;i<32;++i)maxTimers.call("timer_create",{{"label","Timer"},{"seconds",1}},0);
    fails(-32004,[&]{maxTimers.call("timer_create",{{"label","Overflow"},{"seconds",1}},0);});
    check(QJsonDocument(maxTimers.call("timers",{},0)).toJson(QJsonDocument::Compact).size()<58000);
    Service restartedTimers(dir.filePath("max-timers.json"));check(restartedTimers.call("list",{},100000000)["total"]==32);
    check(!restartedTimers.nextDeadline());restartedTimers.advance(200000000);check(restartedTimers.call("list",{},200000000)["total"]==32);
    std::cout<<checks<<" productivity checks passed\n"; return 0;
 } catch(const std::exception &e) { std::cerr<<e.what()<<"\n"; return 1; }
}
