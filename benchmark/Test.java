public class Test {
  public static void main(String[] args) {
    long startMilis = System.currentTimeMillis();
    
    long product = 1;
    for (int i = 0; i < 1000000; i++) {
      product = 1;
      for (int j = 1; j <= 16; j++) {
        product *= j;
      }
    }
    
    System.out.printf("Time used: %d ms\n", System.currentTimeMillis() - startMilis);
    System.out.printf("Result: %d\n", product);
  }
}
