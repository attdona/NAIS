PROTO_VERSION=3.3.0

curl -OL https://github.com/google/protobuf/releases/download/v${PROTO_VERSION}/protoc-${PROTO_VERSION}-linux-x86_64.zip

unzip -o protoc-${PROTO_VERSION}-linux-x86_64.zip -d ${HOME}/protoc3
rm protoc-${PROTO_VERSION}-linux-x86_64.zip