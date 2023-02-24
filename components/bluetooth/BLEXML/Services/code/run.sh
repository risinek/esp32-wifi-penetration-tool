#!/bin/bash
BASE_URL="https://www.bluetooth.com/api/gatt/XmlFile?xmlFileName="
RESULT=services.json
COUNT=0
echo -e "[\n" > ${RESULT}
for fileName in `cat services.txt`
do
	echo "Process file ${fileName}"
	wget --output-document ${fileName}.xml --quiet "${BASE_URL}${fileName}.xml"
	if [ ${COUNT} -gt 0 ]
	then
	   echo -e ",\n" >> ${RESULT}
	fi
	#xml2json < "${fileName}.xml" >> ${RESULT}
	xml2json "${fileName}.xml" "${fileName}.json"
	cat "${fileName}.json" >> ${RESULT}
	COUNT=$(expr ${COUNT} + 1)
done
echo -e "\n]\n" >> ${RESULT}
echo "done"
