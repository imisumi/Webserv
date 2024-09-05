#include <stdio.h>


// int main(int argc, char *argv[], char *envp[])
// {
// 	printf("Content-type: text/json\r\n");
// 	printf("\r\n");
// 	for (int i = 0; envp[i] != NULL; i++)
// 	{
// 		printf("%s\n", envp[i]);
// 	}

// 	return 0;
// }

// #include <cstdio>

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[])
{
	// Output the HTTP header for JSON content type
	printf("Content-type: application/json\r\n");
	printf("\r\n");

	// Begin the JSON object
	printf("{\n");

	// Iterate through the environment variables
	for (int i = 0; envp[i] != NULL; i++)
	{
		// Find the '=' character in the environment variable
		char *equalSign = strchr(envp[i], '=');
		if (equalSign != NULL)
		{
			*equalSign = '\0'; // Temporarily terminate the key string
			const char *key = envp[i];
			const char *value = equalSign + 1;

			// Print key and value in JSON format
			printf("  \"%s\": \"%s\"", key, value);

			// Add a comma if not the last item
			if (envp[i + 1] != NULL)
			{
				printf(",\n");
			}
			else
			{
				printf("\n");
			}

			*equalSign = '='; // Restore the original string
		}
	}

	// End the JSON object
	printf("}\n");

	return 0;
}
