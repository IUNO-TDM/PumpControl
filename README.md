# PumpControl
An executable for decrypting and running recipe programs on the cocktail mixer



The format of the recipe:

Program {
  id:	
    string
  lines:	
  [
    {
      components:	
      [
        {
          ingredient:	
            string
          amount:	
            number
        }
      ]
      timing:	
        integer
        0: machine can decide, 1: all ingredients as fast as possible, 2: all beginning as early as possible and end with the slowest together, 3: one after the other
      sleep:	
        integer
        the sleep time after the line in ms
     }
  ]
}
