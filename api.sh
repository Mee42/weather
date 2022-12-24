curl $(cat secrets.h  | grep API | head -n 1 | cut -b 25- | rev | cut -b 2- | rev) | jq
