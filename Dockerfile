FROM ubuntu:xenial

RUN apt-get update && apt-get install -y net-tools python3-pip ser2net unzip vim wget
RUN wget https://github.com/google/protobuf/releases/download/v3.3.0/protoc-3.3.0-linux-x86_64.zip; unzip protoc-3.3.0-linux-x86_64.zip -d /usr/local; rm protoc-3.3.0-linux-x86_64.zip 

COPY setup.py app/
COPY pynais app/pynais
COPY recipes app/recipes
COPY commands app/commands

# Set the working directory to /app
WORKDIR /app

RUN pip3 install . 

ENV LC_ALL=C.UTF-8 LANG=C.UTF-8

EXPOSE 2001 3000 3001 3002

ENTRYPOINT ["python3", "recipes/hello-ws-junction.py"]
CMD ["--serial"]
