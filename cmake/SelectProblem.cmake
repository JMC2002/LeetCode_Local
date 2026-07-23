block(PROPAGATE LC_PROBLEM_DIR LC_SOLUTION_FILE LC_CASES_FILE)
    if(NOT LC_ID MATCHES "^[0-9]+$")
        message(FATAL_ERROR "LC_ID 只能包含十进制数字：'${LC_ID}'")
    endif()

    file(GLOB candidates
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES true
        "${CMAKE_CURRENT_SOURCE_DIR}/problems/${LC_ID}.*"
    )

    set(problem_dirs)
    foreach(candidate IN LISTS candidates)
        if(IS_DIRECTORY "${candidate}")
            list(APPEND problem_dirs "${candidate}")
        endif()
    endforeach()

    list(LENGTH problem_dirs problem_count)
    if(problem_count EQUAL 0)
        message(FATAL_ERROR "没有目录匹配 problems/${LC_ID}.*")
    elseif(problem_count GREATER 1)
        list(JOIN problem_dirs "\n  " matches)
        message(FATAL_ERROR
            "题号 ${LC_ID} 对应多个目录：\n  ${matches}"
        )
    endif()

    list(GET problem_dirs 0 LC_PROBLEM_DIR)
    set(LC_SOLUTION_FILE "${LC_PROBLEM_DIR}/solution.cpp")
    set(LC_CASES_FILE "${LC_PROBLEM_DIR}/cases.txt")

    foreach(problem_file IN ITEMS "${LC_SOLUTION_FILE}" "${LC_CASES_FILE}")
        if(NOT EXISTS "${problem_file}")
            message(FATAL_ERROR "缺少题目文件：${problem_file}")
        endif()
    endforeach()
endblock()
