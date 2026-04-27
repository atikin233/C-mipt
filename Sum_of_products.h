#include <iostream>

void Deletion(int* indexes, int* lengths, int** all_numbers, int k) {
  for (int i = 0; i < k; ++i) {
    delete[] all_numbers[i];
  }
  delete[] indexes;
  delete[] lengths;
  delete[] all_numbers;
}

bool Difference(int k, int* indexes) {
  for (int i = 0; i < k; ++i) {
    for (int j = i + 1; j < k; ++j) {
      if (indexes[i] == indexes[j]) {
        return false;
      }
    }
  }
  return true;
}

bool Permutations(int k, int* indexes, int* lengths, int i = 0) {
  if (i == k) {
    return false;
  }

  ++indexes[i];

  if (indexes[i] < lengths[i]) {
    return true;
  }

  indexes[i] = 0;

  return Permutations(k, indexes, lengths, i + 1);
}

long long Multiplication(int k, int* indexes, int** all_numbers) {
  long long result_of_multiplication = 1;
  for (int i = 0; i < k; ++i) {
    result_of_multiplication =
        result_of_multiplication * all_numbers[i][indexes[i]];
  }
  return result_of_multiplication;
}

int main(int argc, char** argv) {
  int k = argc - 1;
  int* lengths = new int[k];
  int* indexes = new int[k]{};

  for (int i = 0; i < k; ++i) {
    lengths[i] = std::atoi(argv[i + 1]);
  }

  int** all_numbers = new int*[k];

  for (int i = 0; i < k; ++i) {
    int* numbers = new int[lengths[i]];

    for (int j = 0; j < lengths[i]; ++j) {
      std::cin >> numbers[j];
    }

    all_numbers[i] = numbers;
  }

  long long result = 0;

  while (true) {
    if (Difference(k, indexes)) {
      result = result + Multiplication(k, indexes, all_numbers);
    }
    if (!Permutations(k, indexes, lengths)) {
      break;
    }
  }

  std::cout << result << '\n';

  Deletion(indexes, lengths, all_numbers, k);
}
