include(ExternalProject)
include(GNUInstallDirs)

if (BUILD_GM)
	if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(TASSL_CONFIG_COMMAND sh ./Configure darwin64-x86_64-cc)
	else()
		set(TASSL_CONFIG_COMMAND bash config -Wl,--rpath=./ shared)
	endif ()

	set(TASSL_BUILD_COMMAND make)

	ExternalProject_Add(tassl
		PREFIX ${CMAKE_SOURCE_DIR}/deps
		DOWNLOAD_NO_PROGRESS 1
		URL https://github.com/FISCO-BCOS/FISCO-BCOS/raw/master/deps/src/TASSL-master.zip
		URL_HASH SHA256=5dd14fcfe070a0c9d3e7d9561502e277e1905a8ce270733bf2884f0e2c0c8d97
		BUILD_IN_SOURCE 1
		CONFIGURE_COMMAND ${TASSL_CONFIG_COMMAND}
		LOG_CONFIGURE 1
		BUILD_COMMAND ${TASSL_BUILD_COMMAND}
		INSTALL_COMMAND ""
	)

	ExternalProject_Get_Property(tassl SOURCE_DIR)
	add_library(TASSL STATIC IMPORTED)
	set(TASSL_SUFFIX .a)
	set(TASSL_INCLUDE_DIRS ${SOURCE_DIR}/include)
	set(TASSL_LIBRARY ${SOURCE_DIR}/libssl${TASSL_SUFFIX})
	set(TASSL_CRYPTO_LIBRARIE ${SOURCE_DIR}/libcrypto${TASSL_SUFFIX})
	set(TASSL_LIBRARIES ${TASSL_LIBRARY} ${TASSL_CRYPTO_LIBRARIE})
	set_property(TARGET TASSL PROPERTY IMPORTED_LOCATION ${TASSL_LIBRARIES})
	set_property(TARGET TASSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${TASSL_INCLUDE_DIRS})
endif()

message(STATUS "TASSL headers: ${TASSL_INCLUDE_DIRS}")
message(STATUS "TASSL lib   : ${TASSL_LIBRARIES}")
