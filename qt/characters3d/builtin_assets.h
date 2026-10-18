#pragma once
#include <QString>
#include <array>

namespace pets::character3d {
// Fixed trusted deployment identities; package input never supplies a QML path.
struct BuiltinAsset {
 const char* asset;
 const char* id;
 const char* type;
 bool quadruped;
 bool catalogueStyle;
 const char* disposition;
};
inline constexpr std::array<BuiltinAsset,28> builtinAssets{{
 {"werewolf","werewolf-3d","Werewolf",false,false,"legacy"},
 {"raven","raven-3d","Raven",false,false,"legacy"},
 {"marmalade","marmalade-3d","Marmalade",true,true,"legacy"},
 {"pip","pip-3d","Pip",true,true,"friendly"},
 {"biscuit","biscuit-3d","Biscuit",true,true,"calm"},
 {"clover","clover-3d","Clover",true,true,"cautious"},
 {"mossback","mossback-3d","Mossback",true,true,"calm"},
 {"rivet","rivet-3d","Rivet",false,true,"curious"},
 {"paper-heron","paper-heron-3d","PaperHeron",false,true,"reserved"},
 {"acorn-custodian","acorn-custodian-3d","AcornCustodian",false,true,"curious"},
 {"puddle","puddle-3d","Puddle",false,true,"friendly"},
 {"pocket-observatory","pocket-observatory-3d","PocketObservatory",false,true,"reserved"},
 {"night-librarian","night-librarian-3d","NightLibrarian",false,true,"reserved"},
 {"sprig","sprig-3d","Sprig",false,true,"calm"},
 {"hearthling","hearthling-3d","Hearthling",false,true,"calm"},
 {"crumb-golem","crumb-golem-3d","CrumbGolem",false,true,"friendly"},
 {"nell","nell-3d","Nell",false,true,"reserved"},
 {"alden","alden-3d","Alden",false,true,"calm"},
 {"saffron","saffron-3d","Saffron",false,true,"curious"},
 {"tess","tess-3d","Tess",false,true,"curious"},
 {"rowan","rowan-3d","Rowan",false,true,"cautious"},
 {"vesper","vesper-3d","Vesper",false,true,"reserved"},
 {"iona","iona-3d","Iona",false,true,"curious"},
 {"milo","milo-3d","Milo",false,true,"friendly"},
 {"orris","orris-3d","Orris",false,true,"calm"},
 {"fen","fen-3d","Fen",false,true,"curious"},
 {"oona","oona-3d","Oona",false,true,"curious"},
 {"quillan","quillan-3d","Quillan",false,true,"reserved"}
}};
inline const BuiltinAsset* builtinAsset(const QString& asset) {
 for(const auto& entry:builtinAssets)if(asset==QLatin1String(entry.asset))return &entry;
 return nullptr;
}
inline const BuiltinAsset* builtinIdentity(const QString& id) {
 for(const auto& entry:builtinAssets)if(id==QLatin1String(entry.id))return &entry;
 return nullptr;
}
inline const BuiltinAsset* builtinSkin(const QString& skin) {
 for(const auto& entry:builtinAssets)if(skin==QLatin1String(entry.type)+QStringLiteral("Skin"))return &entry;
 return nullptr;
}
}
