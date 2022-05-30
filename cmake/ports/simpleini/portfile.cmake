# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/simpleini
    REF d63886d4ba8e9e9533a87bff3f90371cfed2ef63
    SHA512 0d2b13164f629cc33e5de1a2f69acd7e7426147c22c6ee2da5b449197f16e5e989af2c0fb344b6a5136bd10291bb82defde37a8e9a7dd942a23846494bed6b1c
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
