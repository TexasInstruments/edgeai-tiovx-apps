#!/bin/bash

toCsv() {

  local file=$1 columnHeaders

  if [[ $SOC == "j784s4" ]]; then
	  if [[ $file =~ "video" ]]; then
	      cut -d':' -f1 "$file" | head -n 13 | paste -d, - - - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - - - -
	  else
	      cut -d':' -f1 "$file" | head -n 15 | paste -d, - - - - - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - - - - - -
	  fi
  elif [[ $SOC == "j721s2" ]]; then
	  if [[ $file =~ "video" ]]; then
	      cut -d':' -f1 "$file" | head -n 11 | paste -d, - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - -
	  else
	      cut -d':' -f1 "$file" | head -n 12 | paste -d, - - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - - -
	  fi
  elif [[ $SOC == "j721e" ]]; then
	  if [[ $file =~ "video" ]]; then
	      cut -d':' -f1 "$file" | head -n 10 | paste -d, - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - -
	  else
	      cut -d':' -f1 "$file" | head -n 11 | paste -d, - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - -
	  fi
  elif [[ $SOC == "j722s" ]]; then
	  if [[ $file =~ "video" ]]; then
	      cut -d':' -f1  "$file" | head -n 11 | paste -d, - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - -
	  else
	      cut -d':' -f1  "$file" | head -n 12 | paste -d, - - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - - -
	  fi
  elif [[ $SOC == "am62a" ]]; then
	  if [[ $file =~ "video" ]]; then
	      cut -d':' -f1 "$file" | head -n 11 | paste -d, - - - - - - - - - - -
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - -
	  else
	      cut -d':' -f1 "$file" | head -n 12 | paste -d, - - - - - - - - - - - - 
	      cut -d':' -f2- "$file" | paste -d, - - - - - - - - - - - -
	  fi
  fi
}

find ./ -maxdepth 1 | grep -E "txt" | xargs sed -i 's/=/:/g'

toCsv ${SOC}_video_tiovx_apps.txt > tiovx_apps_video_${SOC}.csv
toCsv ${SOC}_camera_tiovx_apps.txt > tiovx_apps_camera_${SOC}.csv

rm -rf ${SOC}_video_tiovx_apps.txt ${SOC}_camera_tiovx_apps.txt

sed -i '2,5s/.$//' tiovx_apps_video_${SOC}.csv
sed -i '2,5s/.$//' tiovx_apps_camera_${SOC}.csv
