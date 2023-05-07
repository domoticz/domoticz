# helm-chart for Domoticz

This is a helm-chart for deploy Domoticz on any kubernetes cluster.  This helm deploy:
 * Internal kubernetes service (svc)
 * Deployment with a pod (container) with Domoticz base. (stable)
 * AWS LoadBalancer to access using UI Domotic instance. 

### How to deploy it
	helm install -n domoticz --create-namespace domoticz .

### Tested with

|Soft 		| version | 
|---		|--- 	  |
|Helm           |v3.x.x	  |
|Kubernetes     |1.20/1.22|
