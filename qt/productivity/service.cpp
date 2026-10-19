#include "service.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace pets::productivity {
namespace {
constexpr qint64 MaxInteger=9007199254740991LL;
constexpr qint64 MaxEpoch=253402300799999LL;
constexpr qint64 MaxSeconds=604800;
[[noreturn]] void invalid(const QString &message) { throw Error(-32602,message); }
qint64 integer(const QJsonObject &o,const QString &key,qint64 low,qint64 high,std::optional<qint64> fallback={}) {
    if (!o.contains(key) && fallback) return *fallback;
    const auto v=o.value(key); const double n=v.toDouble(-1);
    if (!v.isDouble() || !std::isfinite(n) || std::floor(n)!=n || n<double(low) || n>double(high)) invalid("Invalid integer: "+key);
    return qint64(n);
}
QString string(const QJsonObject &o,const QString &key,int maxBytes,bool required=true) {
    if (!o.contains(key) && !required) return {};
    if (!o[key].isString()) invalid("Invalid string: "+key);
    const QString text=o[key].toString();
    if ((required && text.isEmpty()) || text.toUtf8().size()>maxBytes) invalid("Invalid length: "+key);
    for(auto c:text.toUcs4()) if (c<32 || (c>=127 && c<=159) || c==0x2028 || c==0x2029 || c==0xfffd || (c>=0xd800 && c<=0xdfff)) invalid("Control or invalid character: "+key);
    return text;
}
QString identifier(const QJsonObject &o,const QString &key) {
    QString value=string(o,key,64);
    static const QRegularExpression grammar("^[A-Za-z0-9_.-]{1,64}$");
    if(!grammar.match(value).hasMatch()) invalid("Invalid identifier: "+key);
    return value;
}
QString title(const QJsonObject &o,const QString &key) {
    const QString value=string(o,key,480);
    if(value.toUcs4().size()>120) invalid("Invalid plain text: "+key);
    return value;
}
QString url(const QJsonObject &o) {
    const QString value=string(o,"source_url",2048,false); if(value.isEmpty()) return {};
    for(int i=0;i<value.size();++i) if(value[i]=='%' && (i+2>=value.size() || !QRegularExpression("^[0-9a-fA-F]{2}$").match(value.mid(i+1,2)).hasMatch())) invalid("Malformed URL escape");
    const QUrl parsed(value,QUrl::StrictMode);
    const QString decoded=QUrl::fromPercentEncoding(value.toUtf8());
    for(auto c:decoded.toUcs4()) if(c<32 || (c>=127 && c<=159)) invalid("URL control character");
    const auto scheme=parsed.scheme();
    if(!parsed.isValid() || parsed.isRelative() || !parsed.userInfo().isEmpty() ||
       !((scheme=="http" || scheme=="https") ? !parsed.host().isEmpty() : scheme=="file" && parsed.authority().isEmpty() && parsed.path().startsWith('/'))) invalid("Unsafe source URL");
    return value;
}
bool boolean(const QJsonObject &o,const QString &key,bool fallback=false) {
    if(!o.contains(key)) return fallback;
    if(!o[key].isBool()) invalid("Invalid boolean: "+key);
    return o[key].toBool();
}
void time(qint64 now) { if(now<0 || now>MaxEpoch-MaxSeconds*1000) invalid("Invalid epoch milliseconds"); }
void validateEvent(const QJsonObject &o) {
    identifier(o,"source_id"); identifier(o,"event_id"); integer(o,"seq",0,MaxInteger);
    const auto kind=string(o,"kind",32);
    if(!QStringList{"working","needs_input","response_ready","completed","failed","cancelled","info"}.contains(kind)) invalid("Invalid event kind");
    title(o,"title"); const auto detail=string(o,"detail",2048,false);
    url(o); if(o.contains("correlation_id")) identifier(o,"correlation_id");
    if(o.contains("occurrence_id")) identifier(o,"occurrence_id");
}
int findItem(const QJsonArray &items,const QString &source,const QString &id) {
    for(int i=0;i<items.size();++i) {auto o=items[i].toObject();if(o["source_id"]==source && o["event_id"]==id) return i;} return -1;
}
void makeRoom(QJsonArray &items) {
    if(items.size()<512) return;
    int oldest=-1;
    for(int i=0;i<items.size();++i) { const auto o=items[i].toObject(); if(!o["completed"].toBool() && !o["acknowledged"].toBool()) continue;
        if(oldest<0 || o["updated_at"].toDouble()<items[oldest].toObject()["updated_at"].toDouble()) oldest=i; }
    if(oldest<0) throw Error(-32004,"Inbox full; acknowledge or complete items first"); items.removeAt(oldest);
}
QJsonObject upsert(QJsonArray &items,const QJsonObject &p,qint64 now,bool internal=false) {
    validateEvent(p); if(!internal && p["source_id"]=="timer") invalid("Reserved source identity");
    const int index=findItem(items,p["source_id"].toString(),p["event_id"].toString());
    QJsonObject o;
    if(index>=0) {o=items[index].toObject();if(p["seq"].toDouble()<=o["seq"].toDouble()) throw Error(-32003,"Stale event sequence");}
    else {makeRoom(items);o={{"created_at",now},{"acknowledged",false},{"completed",false},{"snoozed_until",0}};}
    if(index>=0 && (o["kind"]!=p["kind"] ||
       (p.contains("occurrence_id") && o["occurrence_id"]!=p["occurrence_id"]))) {
        o["acknowledged"]=false;o["completed"]=false;o["snoozed_until"]=0;
    }
    for(const auto &key:{"source_id","event_id","seq","kind","title"}) o[key]=p[key];
    for(const auto &key:{"detail","source_url","correlation_id"}) {
        if(p.contains(key)) o[key]=p[key]; else if(!o.contains(key) && QString(key)!="correlation_id") o[key]="";
    }
    o["source_url"]=p.contains("source_url") ? p["source_url"] : QJsonValue("");
    if(p.contains("occurrence_id")) o["occurrence_id"]=p["occurrence_id"];
    else o.remove("occurrence_id");
    o["updated_at"]=now;
    if(index>=0) items[index]=o; else items.append(o); return o;
}
QJsonObject publicTimer(QJsonObject timer,qint64 now) {
    if(timer["state"]=="running") timer["remaining_ms"]=std::max<qint64>(0,qint64(timer["deadline"].toDouble())-now);
    for(const auto &key:{"work_ms","break_ms","long_break_ms","rounds","seq"}) timer.remove(key);
    return timer;
}
void validateState(const QJsonObject &state) {
    if(integer(state,"version",1,1)!=1 || !state["items"].isArray() || !state["timers"].isArray()) invalid("Invalid storage format");
    auto items=state["items"].toArray(),timers=state["timers"].toArray(); if(items.size()>512 || timers.size()>32) invalid("Storage exceeds capacity");
    QSet<QString> identities;
    for(const auto &v:items) {
        if(!v.isObject()) invalid("Invalid stored item"); auto o=v.toObject();validateEvent(o);
        const auto key=o["source_id"].toString()+":"+o["event_id"].toString(); if(identities.contains(key)) invalid("Duplicate item"); identities.insert(key);
        integer(o,"created_at",0,MaxEpoch);integer(o,"updated_at",0,MaxEpoch);integer(o,"snoozed_until",0,MaxEpoch);
        if(!o.contains("acknowledged") || !o.contains("completed")) invalid("Missing action flags"); boolean(o,"acknowledged");boolean(o,"completed");
    }
    identities.clear();
    for(const auto &v:timers) {
        if(!v.isObject()) invalid("Invalid stored timer");auto o=v.toObject();auto id=identifier(o,"id");if(identities.contains(id)) invalid("Duplicate timer");identities.insert(id);
        title(o,"label");if(!QStringList{"countdown","pomodoro"}.contains(string(o,"mode",16))) invalid("Invalid timer mode");
        if(!QStringList{"work","short_break","long_break"}.contains(string(o,"phase",16))) invalid("Invalid timer phase");
        if(!QStringList{"running","paused","waiting","finished","cancelled"}.contains(string(o,"state",16))) invalid("Invalid timer state");
        integer(o,"remaining_ms",0,MaxSeconds*1000);integer(o,"deadline",0,MaxEpoch);integer(o,"work_intervals",0,MaxInteger);
        integer(o,"work_ms",1000,MaxSeconds*1000);integer(o,"break_ms",1000,MaxSeconds*1000);integer(o,"long_break_ms",1000,MaxSeconds*1000);integer(o,"rounds",1,100);integer(o,"seq",0,MaxInteger);
    }
}
}
Service::Service(const QString &file):file_(file),state_{{"version",1},{"items",QJsonArray{}},{"timers",QJsonArray{}}} {
    QFile input(file); if(!QFileInfo::exists(file)) return;
    try {
        if(!input.open(QIODevice::ReadOnly) || input.size()>4*1024*1024) throw Error(-32010,"Cannot read productivity storage");
        QJsonParseError parse;auto doc=QJsonDocument::fromJson(input.read(4*1024*1024+1),&parse);
        if(parse.error!=QJsonParseError::NoError || !doc.isObject()) throw Error(-32010,"Corrupt productivity storage");
        validateState(doc.object());state_=doc.object();
    } catch(const Error &e) {loadFailed_=true;error_=QString::fromUtf8(e.what());}
}
void Service::persist(const QJsonObject &candidate) {
    if(loadFailed_) throw Error(-32010,error_);
    QSaveFile output(file_);output.setDirectWriteFallback(false);
    auto bytes=QJsonDocument(candidate).toJson(QJsonDocument::Compact);
    if(bytes.size()>4*1024*1024) throw Error(-32004,"Productivity storage capacity reached");
    if(!output.open(QIODevice::WriteOnly) || !output.setPermissions(QFile::ReadOwner|QFile::WriteOwner) || output.write(bytes)!=bytes.size() || !output.commit()) {
        error_="Cannot persist productivity storage: "+output.errorString();throw Error(-32010,error_);
    }
    state_=candidate;error_.clear();
}
QJsonObject Service::call(const QString &operation,const QJsonObject &p,qint64 now) {
    time(now);if(loadFailed_) throw Error(-32010,error_);
    if(!QStringList{"capabilities","list","timers","publish","act","timer_create","timer_act"}.contains(operation))
        throw Error(-32601,"Unknown productivity operation");
    const bool repairs=(operation=="act" && (p["action"]=="acknowledge" || p["action"]=="complete")) ||
                       (operation=="timer_act" && p["action"]=="cancel");
    // Suspend automatic reconciliation after a failure until a successful mutation
    // repairs capacity/storage; reads and acknowledgment remain available.
    if(error_.isEmpty()) {
        try { advance(now); }
        catch(const Error &e) {
            if(!repairs || (e.code!=-32004 && e.code!=-32010)) throw;
        }
    }
    QJsonObject candidate=state_,result; auto items=candidate["items"].toArray(),timers=candidate["timers"].toArray();
    if(operation=="capabilities") return {{"inbox",true},{"timers",true},{"pomodoro",true},{"os_notifications",QJsonObject{{"available",false},{"status","unverified"}}}};
    if(operation=="list") {
        auto query=string(p,"query",512,false);bool attention=boolean(p,"attention_only");int offset=integer(p,"offset",0,512,0),limit=integer(p,"limit",1,50,50);QJsonArray page;int total=0;bool budgetFull=false;
        for(auto it=items.end();it!=items.begin();) {auto o=(*--it).toObject();
            if(attention && (o["kind"]=="working" || o["kind"]=="cancelled" || o["acknowledged"].toBool() || o["completed"].toBool() || o["snoozed_until"].toDouble()>now)) continue;
            if(!query.isEmpty() && !(o["title"].toString()+" "+o["detail"].toString()+" "+o["source_id"].toString()).contains(query,Qt::CaseInsensitive)) continue;
            if(total++<offset || page.size()>=limit || budgetFull) continue;
            auto tentative=page;tentative.append(o);if(QJsonDocument(tentative).toJson(QJsonDocument::Compact).size()<58000) page=tentative;else budgetFull=true;
        }
        return {{"items",page},{"total",total}};
    }
    if(operation=="timers") {QJsonArray out;for(auto t:timers) out.append(publicTimer(t.toObject(),now));return {{"timers",out}};}
    if(operation=="publish") result={{"item",upsert(items,p,now)}};
    else if(operation=="act") {
        int index=findItem(items,identifier(p,"source_id"),identifier(p,"event_id"));if(index<0) invalid("Unknown item");auto o=items[index].toObject();
        if(integer(p,"seq",0,MaxInteger)!=qint64(o["seq"].toDouble())) throw Error(-32003,"Stale action sequence");
        auto action=string(p,"action",20);
        if(action=="acknowledge") o["acknowledged"]=true;
        else if(action=="complete") {o["completed"]=true;o["snoozed_until"]=0;}
        else if(action=="snooze") {o["snoozed_until"]=now+integer(p,"seconds",1,MaxSeconds,300)*1000;}
        else invalid("Unknown item action");o["updated_at"]=now;items[index]=o;result={{"item",o}};
    } else if(operation=="timer_create") {
        QString label=title(p,"label"),mode=string(p,"mode",16,false);if(mode.isEmpty()) mode="countdown";if(mode!="countdown" && mode!="pomodoro") invalid("Invalid timer mode");
        auto work=integer(p,"seconds",1,MaxSeconds)*1000,shortBreak=integer(p,"break_seconds",1,MaxSeconds,300)*1000,longBreak=integer(p,"long_break_seconds",1,MaxSeconds,900)*1000,rounds=integer(p,"rounds",1,100,4);
        if(timers.size()>=32) {int remove=-1;for(int i=0;i<timers.size();++i)if(timers[i].toObject()["state"]=="finished" || timers[i].toObject()["state"]=="cancelled"){remove=i;break;}if(remove<0)throw Error(-32004,"Timer capacity reached");timers.removeAt(remove);}
        QJsonObject t{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"label",label},{"mode",mode},{"phase","work"},{"state","running"},{"remaining_ms",work},{"deadline",now+work},{"work_intervals",0},{"work_ms",work},{"break_ms",shortBreak},{"long_break_ms",longBreak},{"rounds",rounds},{"seq",0}};
        timers.append(t);result={{"timer",publicTimer(t,now)}};
    } else if(operation=="timer_act") {
        auto id=identifier(p,"id"),action=string(p,"action",20);int index=-1;for(int i=0;i<timers.size();++i)if(timers[i].toObject()["id"]==id)index=i;if(index<0)invalid("Unknown timer");auto t=timers[index].toObject();auto status=t["state"].toString();
        if(action=="pause" && status=="running" && t["deadline"].toDouble()>now) {t["remaining_ms"]=qint64(t["deadline"].toDouble())-now;t["deadline"]=0;t["state"]="paused";}
        else if(action=="resume" && status=="paused") {t["deadline"]=now+qint64(t["remaining_ms"].toDouble());t["state"]="running";}
        else if(action=="cancel" && status!="finished" && status!="cancelled") {t["state"]="cancelled";t["deadline"]=0;t["remaining_ms"]=0;}
        else if(action=="next" && status=="waiting" && t["mode"]=="pomodoro") {
            QString phase=t["phase"].toString()=="work" ? (qint64(t["work_intervals"].toDouble())%t["rounds"].toInt()==0 ? "long_break" : "short_break") : "work";
            qint64 duration=qint64(t[phase=="work" ? "work_ms" : phase=="short_break" ? "break_ms" : "long_break_ms"].toDouble());t["phase"]=phase;t["remaining_ms"]=duration;t["deadline"]=now+duration;t["state"]="running";
            int itemIndex=findItem(items,"timer",id);if(itemIndex>=0){auto o=items[itemIndex].toObject();o["acknowledged"]=true;items[itemIndex]=o;}
        } else invalid("Timer action not valid in current state");timers[index]=t;result={{"timer",publicTimer(t,now)}};
    } else throw Error(-32601,"Unknown productivity operation");
    candidate["items"]=items;candidate["timers"]=timers;persist(candidate);
    if(repairs) {
        // The requested action is durable before we retry due events. Failure of
        // that separate reconciliation remains visible through error() and keeps
        // nextDeadline() suspended; it must not falsely reject the saved action.
        try { advance(now); } catch(const Error &) {}
    }
    return result;
}
void Service::advance(qint64 now) {
    try {
    time(now);if(loadFailed_) throw Error(-32010,error_);auto candidate=state_;auto items=candidate["items"].toArray(),timers=candidate["timers"].toArray();bool changed=false;
    for(int i=0;i<items.size();++i) {auto o=items[i].toObject();if(o["snoozed_until"].toDouble()>0 && o["snoozed_until"].toDouble()<=now){o["snoozed_until"]=0;if(!o["completed"].toBool())o["acknowledged"]=false;o["updated_at"]=now;items[i]=o;changed=true;}}
    for(int i=0;i<timers.size();++i) {auto t=timers[i].toObject();if(t["state"]!="running" || t["deadline"].toDouble()>now)continue;
        const auto id=t["id"].toString();auto seq=qint64(t["seq"].toDouble())+1;if(seq>MaxInteger)throw Error(-32004,"Timer sequence exhausted");
        // A new interval is a new attention event even though its kind stays completed.
        int previous=findItem(items,"timer",id);if(previous>=0){auto o=items[previous].toObject();o["acknowledged"]=false;o["completed"]=false;o["snoozed_until"]=0;items[previous]=o;}
        upsert(items,{{"source_id","timer"},{"event_id",id},{"seq",seq},{"kind","completed"},{"title",t["label"]},{"detail",t["mode"]=="pomodoro" ? "Pomodoro "+t["phase"].toString()+" interval finished" : "Timer finished"}},now,true);
        t["seq"]=seq;t["state"]=t["mode"]=="pomodoro" ? "waiting" : "finished";t["deadline"]=0;t["remaining_ms"]=0;
        if(t["phase"]=="work")t["work_intervals"]=qint64(t["work_intervals"].toDouble())+1;timers[i]=t;changed=true;
    }
    if(changed){candidate["items"]=items;candidate["timers"]=timers;persist(candidate);}
    } catch(const Error &e) {error_=QString::fromUtf8(e.what());throw;}
}
std::optional<qint64> Service::nextDeadline() const {
    if(loadFailed_ || !error_.isEmpty()) return {};std::optional<qint64> result;
    auto consider=[&](qint64 n){if(n>0 && (!result || n<*result))result=n;};
    for(auto v:state_["items"].toArray()) consider(qint64(v.toObject()["snoozed_until"].toDouble()));
    for(auto v:state_["timers"].toArray()){auto t=v.toObject();if(t["state"]=="running")consider(qint64(t["deadline"].toDouble()));}
    return result;
}
}
