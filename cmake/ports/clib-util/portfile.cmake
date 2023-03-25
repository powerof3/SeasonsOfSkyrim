# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF 3a3983942b2ff7b74ed4c368192b3965a59b453b
    SHA512 698fab8996e589870ff1147eba9618f0d23b8783b0cdf272c05c3da38f450d34216671024c216da53c94d5e0678a2bc23f730c7ff415dda8afafc544aaa1163f
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
