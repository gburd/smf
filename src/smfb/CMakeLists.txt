add_executable (smfb
  ${PROJECT_SOURCE_DIR}/src/smfb/main.cc
  ${PROJECT_SOURCE_DIR}/src/smfb/smfb_command_line_options.cc
  )
target_link_libraries (smfb smfrpc raft cr)
