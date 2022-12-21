curl $(cat secrets.h  | grep API | cut -b 25- | rev | cut -b 2- | rev) | jq
