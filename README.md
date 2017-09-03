# dogcat
Concatenates stdin or file input and sends it line by line over UDP to a DogStatsD compatible server in Event format

See https://docs.datadoghq.com/guides/dogstatsd/ for protocol description

## Usage example
```
echo "hello world" 2>&1 | ./dogcat 127.0.0.1 8124 -d -o "#domain:live,service:hello" -
```