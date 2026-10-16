import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick3D
import WolfAsset
Window {
 id: window
 property bool showContacts: false
 function screenXForTarget(units) {
  return view.mapFrom3DScene(Qt.vector3d(0,0,units/100)).x * pixelOutput.width/view.width
 }
 function targetAt(screenX) {
  const start=screenXForTarget(-120), end=screenXForTarget(120)
  return Math.max(-120,Math.min(120,-120+240*(screenX-start)/(end-start)))
 }
 width: 768; height: 680; visible: previewVisible; color: "transparent"
 title: "Better Pets — live low-poly werewolf"
 Rectangle { anchors.fill: parent; color: "#18222e"; visible: !captureTransparent }
 View3D {
  id: view; objectName: "view3d"
  width: 192; height: 128
  renderMode: View3D.Offscreen
  explicitTextureWidth: 192; explicitTextureHeight: 128
  environment: SceneEnvironment { backgroundMode: SceneEnvironment.Transparent; antialiasingMode: SceneEnvironment.NoAA }
  OrthographicCamera { id: camera; objectName: "camera"; clipNear: .01; clipFar: 100; position: Qt.vector3d(4, (ravenModel ? 4.9 : 2.7)+motion.elevation, 4); eulerRotation: Qt.vector3d(ravenModel ? -35 : -15,45,0); horizontalMagnification: ravenModel ? 28 : 45; verticalMagnification: ravenModel ? 28 : 45 }
  // Markers use the same world anchors as IK, so turning cannot move a planted dot.
  Model {
   visible: !captureTransparent; source: "#Cube"
   position: Qt.vector3d(0,-.006,0); scale: Qt.vector3d(.00025,.00008,.024)
   materials: PrincipledMaterial { baseColor: "#536578"; lighting: PrincipledMaterial.NoLighting }
  }
  Model {
   objectName: "targetMarker"; visible: !captureTransparent; source: "#Sphere"
   position: Qt.vector3d(0,.025,motion.targetOffset); scale: Qt.vector3d(.0008,.0005,.0008)
   materials: PrincipledMaterial { baseColor: "#f8bd68"; lighting: PrincipledMaterial.NoLighting }
  }
  Model {
   objectName: "leftContact"; visible: window.showContacts && !captureTransparent; source: "#Sphere"
   position: motion.leftContact; scale: Qt.vector3d(.00065,.00065,.00065)
   materials: PrincipledMaterial { baseColor: motion.leftSwing ? "#f8bd68" : "#73e3c3"; lighting: PrincipledMaterial.NoLighting }
  }
  Model {
   objectName: "rightContact"; visible: window.showContacts && !captureTransparent; source: "#Sphere"
   position: motion.rightContact; scale: Qt.vector3d(.00065,.00065,.00065)
   materials: PrincipledMaterial { baseColor: motion.rightSwing ? "#f8bd68" : "#73e3c3"; lighting: PrincipledMaterial.NoLighting }
  }
  Model {
   visible: window.showContacts && !captureTransparent; source: "#Sphere"
   position: motion.leftGrip; scale: Qt.vector3d(.00065,.00065,.00065)
   materials: PrincipledMaterial { baseColor: "#ed91cf"; lighting: PrincipledMaterial.NoLighting }
  }
  Model {
   visible: window.showContacts && !captureTransparent; source: "#Sphere"
   position: motion.rightGrip; scale: Qt.vector3d(.00065,.00065,.00065)
   materials: PrincipledMaterial { baseColor: "#ed91cf"; lighting: PrincipledMaterial.NoLighting }
  }
  DirectionalLight { eulerRotation: Qt.vector3d(-35,-30,0); brightness: .8 }
  Node {
   objectName: "avatar"
   position: Qt.vector3d(0,motion.elevation,motion.rootOffset)
   eulerRotation.y: motion.yaw
   Werewolf { objectName: "werewolf" }
  }
 }
 ShaderEffectSource {
  id: pixelOutput; objectName: "pixelOutput"
  width: 768; height: 512
  sourceItem: view; hideSource: true
  textureSize: Qt.size(192,128)
  smooth: false; live: true
 }
 MouseArea { anchors.fill: pixelOutput; onClicked: mouse => motion.target(window.targetAt(mouse.x)) }
 Column {
  x: 18; y: 525; spacing: 8; visible: !captureTransparent
  Row {
   spacing: 10
   CheckBox { text: "Curious"; checked: motion.autonomous; onToggled: motion.autonomous=checked }
   TextField { width: 74; text: motion.seed; placeholderText: "Seed"; validator: IntValidator { bottom: 0 } onEditingFinished: motion.seed=Number(text) }
   Text { anchors.verticalCenter: parent.verticalCenter; color: "white"; text: "Travel speed" }
   Slider {
    objectName: "walkingSpeed"; width: 190
    from: .5; to: 3; stepSize: .1; value: motion.walkingSpeed
    onMoved: motion.walkingSpeed=value
    ToolTip.visible: hovered
    ToolTip.text: "Moves the body at the selected speed. Feet follow and catch up when needed."
   }
   Text { anchors.verticalCenter: parent.verticalCenter; color: "white"; width: 38; text: motion.walkingSpeed.toFixed(1)+"×" }
   Button { text: "Normal 1×"; onClicked: motion.walkingSpeed=1 }
  }
  Text { color: "white"; text: motion.status }
  Text { color: "#b9c7d5"; text: "Click to move • W wave • H hold • R release • L look • E ear • T tail • C clear" }
  Text { color: "#b9c7d5"; text: "192×128 GPU composition • nearest atlas/output • ground / hop / powered flight" }
 }
 property bool paused: false
 Shortcut { sequence: "Left"; onActivated: motion.target(90) }
 Shortcut { sequence: "Right"; onActivated: motion.target(-90) }
 Shortcut { sequence: "Space"; onActivated: motion.stop() }
 Shortcut { sequence: "J"; onActivated: motion.hop(25) }
 Shortcut { sequence: "F"; onActivated: motion.flyTo(40,55) }
 Shortcut { sequence: "G"; onActivated: motion.land() }
 Shortcut { sequence: "I"; onActivated: motion.impulse() }
 Shortcut { sequence: "L"; onActivated: motion.look(Qt.vector3d(.5,.2,1)) }
 Shortcut { sequence: "T"; onActivated: motion.roleGoal("tail",Qt.vector3d(0,.5,0)) }
 Shortcut { sequence: "E"; onActivated: motion.roleGoal("earLeft",Qt.vector3d(0,0,.35)) }
 Shortcut { sequence: "C"; onActivated: motion.clearExpression() }
 Shortcut { sequence: "W"; onActivated: motion.gesture("wave") }
 Shortcut { sequence: "H"; onActivated: motion.reach(0,Qt.vector3d(.6,1.3,.3)) }
 Shortcut { sequence: "R"; onActivated: motion.release(0) }
 Shortcut { sequence: "D"; onActivated: window.showContacts=!window.showContacts }
 Shortcut { sequence: "P"; onActivated: { window.paused=!window.paused; motion.pause(window.paused) } }
}
