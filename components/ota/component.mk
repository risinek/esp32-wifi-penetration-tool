#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# This line is needed in case HTTPS is used for OTA and we need to store server certificate
# Refer to "simple_ota_example"
# COMPONENT_EMBED_TXTFILES :=  ./server_certs/ca_cert.pem
