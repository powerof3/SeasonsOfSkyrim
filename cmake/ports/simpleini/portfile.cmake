# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/simpleini
    REF 248e922e7b233a13ebcd571f14305defea46122b
    SHA512 b57b1795a2058269f7db2f9dce244f561e445421ad89a318f042f739ba76972d906f5c370f040a90b1dae6fb9378e8c432112e69a948d66e53832723419cc1ac
    HEAD_REF master
)

# Install codes
set(SIMPLEINI_SOURCE ${SOURCE_PATH}/SimpleIni.h
                     ${SOURCE_PATH}/ConvertUTF.h
                     ${SOURCE_PATH}/ConvertUTF.c
)

file(INSTALL ${SIMPLEINI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
# Install sample
#file(INSTALL ${SOURCE_PATH}/snippets.cpp DESTINATION ${CURRENT_PACKAGES_DIR}/share/sample)

file(INSTALL ${SOURCE_PATH}/LICENCE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
