
CUR_UID=`id -u`
CUR_GID=`id -g`

function arm() {
  if [ -e /dev/ttyUSB0 ]; then
    DEVICE="--device=/dev/ttyUSB0"
  fi
  if [[ "$OSTYPE" != "darwin"* ]]; then
    USB="-v /dev/bus/usb:/dev/bus/usb"
  fi
  docker run -ti --rm \
    --user $CUR_UID:$CUR_GID \
    --volume=${PWD}:/src/:rw \
    --group-add=dialout \
    --group-add=adm \
    ${DEVICE} \
    --privileged \
    ${USB} \
    --net=host \
    arm "$@"
}

DIR=`dirname $0`
docker build -q -t arm ${DIR}/docker > /dev/null
