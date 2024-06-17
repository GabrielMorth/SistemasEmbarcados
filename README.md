# SistemasEmbarcados
Repositório criado para abrigar os códigos da disciplina de Sistemas Embarcados.

Os códigos estão sem as linhas que conectam o ESP ao servidor, pois ao colocá-las no github, o site derruba o dashboard que controle o motor. El teas serão adicionadas depois da apresentação da disciplina. 

No repositório tem 3 códigos:
V4: É o código que está no ESP no momento da apresentação. Ele não possui RTOS e o visor da balança atualiza a cada loop, dificultando a leitura. 
V5: O problema da balança foi alterado. Nele, a balança só é alterada se a leitura muda em 10% com relação ao valor do loop anterior. 
V6: É o código pronto, com RTOS.

Não foi possível testar os códigos V5 e V6, pois encontramos um problema no upload do código no ESP e não foi possível solucionar o problema a tempo. Apenas foi checado que os códigos V5 e V6 compilam. 
