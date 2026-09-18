#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 10

typedef struct {
    int id;
    char name[50];
    int marks;
} Student;

/* Compare function for qsort */
int compare_students(const void *a, const void *b) {
    const Student *s1 = (const Student *)a;
    const Student *s2 = (const Student *)b;

    return s2->marks - s1->marks;
}

/* Recursive factorial */
long long factorial(int n) {
    if (n <= 1)
        return 1;

    return n * factorial(n - 1);
}

/* Recursive Fibonacci */
long long fibonacci(int n) {
    if (n <= 1)
        return n;

    return fibonacci(n - 1) + fibonacci(n - 2);
}

/* Check whether number is prime */
int is_prime(int n) {
    if (n < 2)
        return 0;

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0)
            return 0;
    }

    return 1;
}

/* Reverse an array using pointers */
void reverse_array(int *arr, int n) {
    int *left = arr;
    int *right = arr + n - 1;

    while (left < right) {
        int temp = *left;
        *left = *right;
        *right = temp;

        left++;
        right--;
    }
}

/* Calculate average using pointer */
double average(int *arr, int n) {
    int sum = 0;

    for (int i = 0; i < n; i++) {
        sum += *(arr + i);
    }

    return (double)sum / n;
}

int main(void) {

    printf("===== COMPLEX C TEST PROGRAM =====\n\n");

    /* ------------------------------------------------ */
    /* 1. Dynamic memory allocation                     */
    /* ------------------------------------------------ */

    int n = 10;

    int *numbers = malloc(n * sizeof(int));

    if (numbers == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("Numbers:\n");

    for (int i = 0; i < n; i++) {
        numbers[i] = (i + 1) * (i + 1);
        printf("%d ", numbers[i]);
    }

    printf("\n\n");


    /* ------------------------------------------------ */
    /* 2. Pointer-based average                        */
    /* ------------------------------------------------ */

    double avg = average(numbers, n);

    printf("Average = %.2f\n\n", avg);


    /* ------------------------------------------------ */
    /* 3. Reverse using pointers                       */
    /* ------------------------------------------------ */

    reverse_array(numbers, n);

    printf("Reversed array:\n");

    for (int i = 0; i < n; i++) {
        printf("%d ", numbers[i]);
    }

    printf("\n\n");


    /* ------------------------------------------------ */
    /* 4. Prime number calculation                     */
    /* ------------------------------------------------ */

    printf("Prime numbers between 1 and 50:\n");

    for (int i = 1; i <= 50; i++) {

        if (is_prime(i)) {
            printf("%d ", i);
        }
    }

    printf("\n\n");


    /* ------------------------------------------------ */
    /* 5. Recursion                                    */
    /* ------------------------------------------------ */

    printf("Factorial(10) = %lld\n", factorial(10));

    printf("Fibonacci(15) = %lld\n\n", fibonacci(15));


    /* ------------------------------------------------ */
    /* 6. Structures + dynamic memory                  */
    /* ------------------------------------------------ */

    Student *students = malloc(5 * sizeof(Student));

    if (students == NULL) {
        free(numbers);
        return 1;
    }

    students[0].id = 101;
    strcpy(students[0].name, "Alice");
    students[0].marks = 87;

    students[1].id = 102;
    strcpy(students[1].name, "Bob");
    students[1].marks = 94;

    students[2].id = 103;
    strcpy(students[2].name, "Charlie");
    students[2].marks = 76;

    students[3].id = 104;
    strcpy(students[3].name, "David");
    students[3].marks = 91;

    students[4].id = 105;
    strcpy(students[4].name, "Eve");
    students[4].marks = 88;


    /* ------------------------------------------------ */
    /* 7. Sorting structures using qsort               */
    /* ------------------------------------------------ */

    qsort(
        students,
        5,
        sizeof(Student),
        compare_students
    );

    printf("Students sorted by marks:\n");

    for (int i = 0; i < 5; i++) {

        printf(
            "ID: %d | Name: %-10s | Marks: %d\n",
            students[i].id,
            students[i].name,
            students[i].marks
        );
    }

    printf("\n");


    /* ------------------------------------------------ */
    /* 8. Matrix calculation                           */
    /* ------------------------------------------------ */

    int matrix[SIZE][SIZE];

    int diagonal_sum = 0;

    for (int i = 0; i < SIZE; i++) {

        for (int j = 0; j < SIZE; j++) {

            matrix[i][j] = (i + 1) * (j + 1);

            if (i == j) {
                diagonal_sum += matrix[i][j];
            }
        }
    }

    printf("10x10 multiplication matrix:\n");

    for (int i = 0; i < SIZE; i++) {

        for (int j = 0; j < SIZE; j++) {
            printf("%4d ", matrix[i][j]);
        }

        printf("\n");
    }

    printf("\n");

    printf("Main diagonal sum = %d\n\n", diagonal_sum);


    /* ------------------------------------------------ */
    /* 9. String processing                            */
    /* ------------------------------------------------ */

    char text[] = "C Programming Code Execution Server";

    int vowels = 0;
    int consonants = 0;

    for (int i = 0; text[i] != '\0'; i++) {

        char c = text[i];

        if (
            c == 'a' || c == 'e' ||
            c == 'i' || c == 'o' ||
            c == 'u' ||
            c == 'A' || c == 'E' ||
            c == 'I' || c == 'O' ||
            c == 'U'
        ) {
            vowels++;
        }
        else if (
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')
        ) {
            consonants++;
        }
    }

    printf("String: %s\n", text);
    printf("Vowels: %d\n", vowels);
    printf("Consonants: %d\n\n", consonants);


    /* ------------------------------------------------ */
    /* 10. Cleanup                                    */
    /* ------------------------------------------------ */

    fre(numbers);
    free(students);

    printf("===== TEST COMPLETED SUCCESSFULLY =====\n");

    return 0;
}