if(APPLE)
    function(PostBuildMacBundle target framework_list lib_list)
        INCLUDE(BundleUtilities)
        GET_TARGET_PROPERTY(_BIN_NAME ${target} LOCATION)
        GET_DOTAPP_DIR(${_BIN_NAME} _BUNDLE_DIR)

        set(_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}_prep.cmake")
        file(WRITE ${_SCRIPT_FILE}
            "# Generated Script file\n"
            "include(BundleUtilities)\n"
            "get_bundle_and_executable(\"\${BUNDLE_APP}\" bundle executable valid)\n"
            "if(valid)\n"
            "  set(framework_dest \"\${bundle}/Contents/Frameworks\")\n"
            "  foreach(framework_path ${framework_list})\n"
            "    get_filename_component(framework_name \${framework_path} NAME_WE)\n"
            "    file(MAKE_DIRECTORY \"\${framework_dest}/\${framework_name}.framework/Versions/A/\")\n"
            "    copy_resolved_framework_into_bundle(\${framework_path}/Versions/A/\${framework_name} \${framework_dest}/\${framework_name}.framework/Versions/A/\${framework_name})\n"
            "  endforeach()\n"
            "  foreach(lib ${lib_list})\n"
            "    get_filename_component(lib_file \${lib} NAME)\n"
            "    copy_resolved_item_into_bundle(\${lib} \${framework_dest}/\${lib_file})\n"
            "  endforeach()\n"
            "else()\n"
            "  message(ERROR \"App Not found? \${BUNDLE_APP}\")\n"
            "endif()\n"
            "#fixup_bundle(\"\${BUNDLE_APP}\" \"\" \"\${DEP_LIB_DIR}\")\n"
        )

        ADD_CUSTOM_COMMAND(TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -DBUNDLE_APP="${_BUNDLE_DIR}" -P "${_SCRIPT_FILE}"
        )
    endfunction()
else()
    function(PostBuildMacBundle target framework_list lib_list)
        # noop
    endfunction()
endif()