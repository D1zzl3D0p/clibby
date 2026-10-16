add_library(pet_character3d_core pose.cpp)
target_link_libraries(pet_character3d_core PUBLIC pet_character_core)
target_include_directories(pet_character3d_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_executable(character3d_tests tests.cpp)
target_link_libraries(character3d_tests PRIVATE pet_character3d_core)
add_test(NAME character3d_motion COMMAND character3d_tests)
add_executable(character3d_flight_tests flight_tests.cpp)
target_link_libraries(character3d_flight_tests PRIVATE pet_character3d_core)
add_test(NAME character3d_flight COMMAND character3d_flight_tests)
add_executable(character3d_expression_tests expression_tests.cpp)
target_link_libraries(character3d_expression_tests PRIVATE pet_character3d_core)
add_test(NAME character3d_expression COMMAND character3d_expression_tests)

if(NOT TARGET Qt6::Quick3D OR NOT TARGET Qt6::ShaderTools OR NOT TARGET Qt6::QuickTimeline)
  message(STATUS "Optional Quick3D/ShaderTools/QuickTimeline unavailable; 3D motion tests only")
  return()
endif()
set(WOLF_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../characters/werewolf-3d/werewolf.glb")
set(WOLF_RIG "${CMAKE_CURRENT_SOURCE_DIR}/../../characters/werewolf-3d/rig.json")
if(EXISTS "${WOLF_SOURCE}" AND EXISTS "${WOLF_RIG}")
  get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(QT_BIN "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
  find_program(PET_BALSAM balsam HINTS "${QT_BIN}" NO_DEFAULT_PATH)
  if(NOT PET_BALSAM)
    message(STATUS "Balsam unavailable; 3D motion tests only")
    return()
  endif()
  set(WOLF_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/converted")
  add_custom_command(OUTPUT "${WOLF_OUTPUT}/Werewolf.qml" "${WOLF_OUTPUT}/conversion.json"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/convert_werewolf_qt.py"
      --source "${WOLF_SOURCE}" --output "${WOLF_OUTPUT}" --balsam "${PET_BALSAM}"
    DEPENDS "${WOLF_SOURCE}" "${WOLF_RIG}" "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/convert_werewolf_qt.py"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/png_metadata.py")
  add_custom_target(werewolf_qt_asset DEPENDS "${WOLF_OUTPUT}/Werewolf.qml")
  set(CMAKE_AUTOMOC ON)
  add_library(pet_character3d_render ../preview3d/controller.cpp ../preview3d/controller.h ../preview3d/resident_scene.cpp ../preview3d/resident_scene.h)
  target_link_libraries(pet_character3d_render PUBLIC pet_behavior pet_character3d_core Qt6::Quick3D Qt6::Quick Qt6::Qml Qt6::Gui)
  add_dependencies(pet_character3d_render werewolf_qt_asset)
  # Trusted converted resources travel beside either executable. Blender/Balsam
  # are build tools only, and no source-directory paths reach resident code.
  add_custom_target(werewolf_resident_resources ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/better-pets-3d"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${WOLF_OUTPUT}" "${CMAKE_BINARY_DIR}/better-pets-3d/WolfAsset"
    COMMAND ${CMAKE_COMMAND} -E copy "${WOLF_RIG}" "${CMAKE_BINARY_DIR}/better-pets-3d/rig.json"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../preview3d/ResidentStage.qml" "${CMAKE_BINARY_DIR}/better-pets-3d/ResidentStage.qml"
    DEPENDS werewolf_qt_asset)
  add_dependencies(pet_character3d_render werewolf_resident_resources)
  install(DIRECTORY "${WOLF_OUTPUT}/" DESTINATION bin/better-pets-3d/WolfAsset)
  install(FILES "${WOLF_RIG}" "${CMAKE_CURRENT_SOURCE_DIR}/../preview3d/ResidentStage.qml" DESTINATION bin/better-pets-3d)
  add_executable(character_preview_3d ../preview3d/main.cpp)
  target_link_libraries(character_preview_3d PRIVATE pet_character3d_render)
  target_compile_definitions(character_preview_3d PRIVATE WOLF_QML_DIR="${WOLF_OUTPUT}" WOLF_RIG_PATH="${WOLF_RIG}" WOLF_STAGE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../preview3d/Stage.qml")
  add_dependencies(character_preview_3d werewolf_qt_asset)
  set(RAVEN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../characters/raven-3d/raven.glb")
  set(RAVEN_RIG "${CMAKE_CURRENT_SOURCE_DIR}/../../characters/raven-3d/rig.json")
  if(EXISTS "${RAVEN_SOURCE}" AND EXISTS "${RAVEN_RIG}")
    set(RAVEN_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/converted-raven")
    add_custom_command(OUTPUT "${RAVEN_OUTPUT}/Raven.qml" "${RAVEN_OUTPUT}/conversion.json"
      COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/convert_raven_qt.py"
        --source "${RAVEN_SOURCE}" --output "${RAVEN_OUTPUT}" --balsam "${PET_BALSAM}"
      DEPENDS "${RAVEN_SOURCE}" "${RAVEN_RIG}" "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/convert_raven_qt.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/png_metadata.py")
    add_custom_target(raven_qt_asset DEPENDS "${RAVEN_OUTPUT}/Raven.qml")
    add_custom_target(raven_resident_resources ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/better-pets-3d/raven"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${RAVEN_OUTPUT}" "${CMAKE_BINARY_DIR}/better-pets-3d/raven/RavenAsset"
      COMMAND ${CMAKE_COMMAND} -E copy "${RAVEN_RIG}" "${CMAKE_BINARY_DIR}/better-pets-3d/raven/rig.json"
      DEPENDS raven_qt_asset)
    add_dependencies(pet_character3d_render raven_resident_resources)
    add_dependencies(character_preview_3d raven_qt_asset)
    target_compile_definitions(character_preview_3d PRIVATE RAVEN_QML_DIR="${RAVEN_OUTPUT}" RAVEN_RIG_PATH="${RAVEN_RIG}")
    install(DIRECTORY "${RAVEN_OUTPUT}/" DESTINATION bin/better-pets-3d/raven/RavenAsset)
    install(FILES "${RAVEN_RIG}" DESTINATION bin/better-pets-3d/raven)
    add_test(NAME raven3d_joint_binding COMMAND character_preview_3d --character raven --verify-joints)
    add_test(NAME raven3d_presentation COMMAND character_preview_3d --character raven --verify-presentation)
    set_tests_properties(raven3d_joint_binding raven3d_presentation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(character3d_resident_tests ../preview3d/resident_tests.cpp)
  target_link_libraries(character3d_resident_tests PRIVATE pet_character3d_render)
  set_target_properties(character3d_resident_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  add_test(NAME character3d_resident_presentation COMMAND character3d_resident_tests)
  set_tests_properties(character3d_resident_presentation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  add_test(NAME character3d_joint_binding COMMAND character_preview_3d --verify-joints)
  set_tests_properties(character3d_joint_binding PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  add_test(NAME character3d_presentation COMMAND character_preview_3d --verify-presentation)
  set_tests_properties(character3d_presentation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  add_test(NAME character3d_gpu_render COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../../.venv/bin/python" "${CMAKE_CURRENT_SOURCE_DIR}/../preview3d/test_gpu.py" $<TARGET_FILE:character_preview_3d>)
  set_tests_properties(character3d_gpu_render PROPERTIES SKIP_RETURN_CODE 77)
else()
  message(STATUS "3D motion core enabled; Blender GLB pending, live preview target unavailable")
endif()
