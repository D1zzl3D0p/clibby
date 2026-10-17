import QtQuick
import QtQuick3D
Item {
 id: stage
 objectName: "resident3dStage"
 width: ravenModel ? 512 : 384; height: ravenModel ? 448 : 384
 property real logicalPixelsPerMeter: ravenModel || catalogueModel ? 80 : 120
 property real cameraTiltDegrees: catalogueModel ? 15.4 : 12
 // Human faces need a more frontal three-quarter view at the same pixel density.
 property real facingBiasDegrees: catalogueModel ? (motion.reachSupported ? -55 : catalogueFacingBias) : -35
 property point rootFloorAnchor: Qt.point(width/2,height/2+1.1*logicalPixelsPerMeter)
 property bool renderingEnabled: true
 function refresh() { pixelOutput.scheduleUpdate() }
 function project(point) {
  const projected=view.mapFrom3DScene(point)
  return Qt.point(projected.x*width/view.width,projected.y*height/view.height)
 }
 View3D {
  id: view; objectName: "view3d"
  width: catalogueModel ? 96 : 128; height: catalogueModel ? 96 : ravenModel ? 112 : 128
  camera: residentCamera
  renderMode: View3D.Offscreen
  explicitTextureWidth: width; explicitTextureHeight: height
  environment: SceneEnvironment { backgroundMode: SceneEnvironment.Transparent; antialiasingMode: SceneEnvironment.NoAA }
  OrthographicCamera {
   id: residentCamera
   objectName: "camera"
   clipNear: .01; clipFar: 100
   // A lightly elevated orthographic view shows the top planes. Scene X is
   // the complete travel axis, so this pitch never moves planted soles up/down
   // as the native window follows. Preserve the calibrated ground projection.
   property real pitchRadians: stage.cameraTiltDegrees*Math.PI/180
   position: Qt.vector3d(0,(1.1+6*Math.sin(pitchRadians))/Math.cos(pitchRadians),6)
   eulerRotation.x: -stage.cameraTiltDegrees
   horizontalMagnification: logicalPixelsPerMeter*Math.max(1,view.width)/Math.max(1,stage.width)
   verticalMagnification: logicalPixelsPerMeter*Math.max(1,view.height)/Math.max(1,stage.height)
  }
  DirectionalLight {
   eulerRotation: Qt.vector3d(-35,-30,0)
   brightness: catalogueModel ? 1.4 : .8
   ambientColor: catalogueModel ? "#888888" : "#000000"
  }
  DirectionalLight { visible: catalogueModel; eulerRotation: Qt.vector3d(-25,145,0); brightness: .65 }
  Node {
   id: basis
   eulerRotation.y: 90
   Node {
    objectName: "avatar"
    // Model navigation remains absolute. The host follows that projected root;
    // applying it locally too would count every travel displacement twice.
    eulerRotation.y: motion.yaw
    Loader3D { source: assetSource }
   }
  }
 }
 ShaderEffectSource {
  id: pixelOutput; objectName: "pixelOutput"
  anchors.fill: parent
  sourceItem: view; hideSource: true
  textureSize: Qt.size(view.width,view.height)
  smooth: false; live: stage.renderingEnabled
 }
}
