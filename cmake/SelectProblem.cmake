block(PROPAGATE LC_PROBLEM_FOUND LC_PROBLEM_DIR LC_SOLUTION_FILE LC_CASES_FILE)
    set(LC_PROBLEM_FOUND TRUE)
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
    if(problem_count EQUAL 0 AND LC_AUTO_FETCH_MISSING)
        if(NOT Python3_Interpreter_FOUND)
            message(FATAL_ERROR
                "没有目录匹配 problems/${LC_ID}.*，自动拉取需要 Python 3"
            )
        endif()

        message(STATUS "本地没有力扣题目 ${LC_ID}，正在自动拉取")
        execute_process(
            COMMAND
                "${Python3_EXECUTABLE}" "${LC_FETCH_SCRIPT}" "${LC_ID}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE fetch_result
            OUTPUT_VARIABLE fetch_output
            ERROR_VARIABLE fetch_error
            ECHO_OUTPUT_VARIABLE
            ECHO_ERROR_VARIABLE
        )
        if(NOT fetch_result EQUAL 0)
            message(FATAL_ERROR
                "自动拉取力扣题目 ${LC_ID} 失败，退出码 ${fetch_result}"
            )
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
    endif()

    if(problem_count EQUAL 0)
        set(LC_PROBLEM_FOUND FALSE)
    elseif(problem_count GREATER 1)
        list(JOIN problem_dirs "\n  " matches)
        message(FATAL_ERROR
            "题号 ${LC_ID} 对应多个目录：\n  ${matches}"
        )
    else()
        list(GET problem_dirs 0 LC_PROBLEM_DIR)
        set(LC_SOLUTION_FILE "${LC_PROBLEM_DIR}/solution.cpp")
        set(LC_CASES_FILE "${LC_PROBLEM_DIR}/cases.txt")

        foreach(problem_file IN ITEMS "${LC_SOLUTION_FILE}" "${LC_CASES_FILE}")
            if(NOT EXISTS "${problem_file}")
                message(FATAL_ERROR "缺少题目文件：${problem_file}")
            endif()
        endforeach()
    endif()
endblock()
