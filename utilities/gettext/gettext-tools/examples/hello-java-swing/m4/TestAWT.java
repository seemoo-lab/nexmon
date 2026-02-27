// Test for working AWT.
public class TestAWT {
  public static void main (String[] args) {
    try {
      java.awt.Toolkit.getDefaultToolkit();
    } catch (Throwable e) {
      System.exit(1);
    }
    System.exit(0);
  }
}
